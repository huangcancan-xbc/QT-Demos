#!/usr/bin/env python3
"""Qt 后端工作脚本：下载单个视频到指定目录。

设计成命令行脚本（CLI），便于 C++/Qt 通过 QProcess 启动并读取输出。
"""

from __future__ import annotations

import argparse
import pathlib
import shutil
import sys
from typing import Any

# Windows 默认控制台编码可能是 gbk，遇到非常规字符会在 print() 时抛 UnicodeEncodeError。
# 将错误策略改为 replace，避免“下载成功但日志输出失败”导致脚本返回非 0 退出码。
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(errors="replace")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(errors="replace")


def log(message: str) -> None:
    """输出一行日志并立即刷新，保证 Qt 日志面板“实时可见”."""
    print(message, flush=True)


def human_size(num_bytes: int | float | None) -> str:
    """把字节数转换为更容易阅读的单位字符串。"""
    if num_bytes is None:
        return "未知"

    size = float(num_bytes)
    units = ["B", "KB", "MB", "GB", "TB"]
    for unit in units:
        if size < 1024.0 or unit == units[-1]:
            return f"{size:.2f} {unit}"
        size /= 1024.0

    return f"{size:.2f} TB"


def build_parser() -> argparse.ArgumentParser:
    # C++ 侧通过命令行参数把 URL 和输出目录传进来。
    # 这里定义参数契约：两者都是必填项。
    parser = argparse.ArgumentParser(description="使用 yt-dlp 下载视频")
    parser.add_argument("--url", required=True, help="要下载的视频链接")
    parser.add_argument("--output-dir", required=True, help="输出目录")
    return parser


def build_ydl_options(output_dir: pathlib.Path, has_ffmpeg: bool) -> dict[str, Any]:
    """构建 yt-dlp 参数，并注入进度钩子用于 UI 实时日志。

    参数:
        output_dir: 下载文件输出目录
        has_ffmpeg: 当前环境是否可用 ffmpeg
    """

    def progress_hook(data: dict[str, Any]) -> None:
        # yt-dlp 会在下载过程反复回调此函数，data 中包含当前阶段信息。
        # 常见字段：
        # - status: downloading / finished
        # - downloaded_bytes: 已下载字节数
        # - total_bytes / total_bytes_estimate: 总大小（可能未知）
        # - speed: 当前下载速度（字节/秒）
        # - eta: 预计剩余秒数
        status = data.get("status")

        if status == "downloading":
            downloaded = human_size(data.get("downloaded_bytes"))
            total = human_size(data.get("total_bytes") or data.get("total_bytes_estimate"))
            speed = human_size(data.get("speed"))
            eta = data.get("eta")

            eta_text = f"{eta}s" if eta is not None else "未知"
            log(
                "下载中："
                f"{downloaded}/{total}, "
                f"速度={speed}/s, "
                f"预计剩余={eta_text}"
            )
            return

        if status == "finished":
            filename = data.get("filename", "<未知文件>")
            log(f"下载完成：{filename}")

    # 格式策略说明：
    # - 有 ffmpeg: 可下分离流并合并，画质通常更高
    # - 无 ffmpeg: 回退到单文件流，兼容性更好，不依赖外部合并器
    format_selector = "bv*+ba/b" if has_ffmpeg else "b"

    return {
        # 安装 ffmpeg 时优先“视频+音频”合并；未安装时降级到最佳单文件。
        "format": format_selector,
        # 文件名模板：限制标题长度，避免 Windows 路径过长等问题。
        "outtmpl": str(output_dir / "%(title).180B [%(id)s].%(ext)s"),
        # 仅下载单个链接，避免 URL 是播放列表时一次性下载过多内容。
        "noplaylist": True,
        "progress_hooks": [progress_hook],
        # 关闭冗余输出，由我们自己控制日志格式。
        "quiet": True,
        "no_warnings": True,
    }


def main() -> int:
    args = build_parser().parse_args()

    # resolve() 把路径规范化为绝对路径，便于日志定位和跨目录启动。
    output_dir = pathlib.Path(args.output_dir).resolve()
    # parents=True: 父目录不存在时一并创建。
    # exist_ok=True: 目录已存在也不报错。
    output_dir.mkdir(parents=True, exist_ok=True)

    log(f"输出目录: {output_dir}")
    log(f"下载链接: {args.url}")

    try:
        import yt_dlp
    except ImportError:
        # 单独返回特定错误码，Qt 侧可据此给出更明确提示。
        log("错误：当前虚拟环境未安装 yt-dlp。")
        log("错误：请先运行 scripts/setup_venv.ps1。")
        return 2

    has_ffmpeg = shutil.which("ffmpeg") is not None
    if has_ffmpeg:
        log("检测到 ffmpeg：将下载最佳视频+音频并自动合并。")
    else:
        log("未检测到 ffmpeg：已降级为最佳单文件流（画质可能略低）。")

    options = build_ydl_options(output_dir, has_ffmpeg)

    try:
        # YoutubeDL 对象是 yt-dlp 的核心入口；with 语法可自动完成资源释放。
        with yt_dlp.YoutubeDL(options) as ydl:
            # download() 返回 0 表示成功，非 0 表示失败。
            result = ydl.download([args.url])

        if result == 0:
            log("成功：视频下载完成。")
            return 0

        log(f"错误：yt-dlp 返回非零状态码：{result}")
        return int(result)
    except Exception as exc:  # noqa: BLE001
        # 兜底异常，确保脚本总能输出可读错误并返回非 0 状态码。
        log(f"错误：下载失败：{exc}")
        return 1


if __name__ == "__main__":
    # 将 main() 的返回值作为进程退出码返回给 Qt。
    # Qt 侧据此判断“成功/失败”并更新状态标签与弹窗。
    sys.exit(main())
