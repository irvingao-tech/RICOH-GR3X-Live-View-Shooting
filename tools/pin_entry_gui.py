import pathlib
import sys
import time
import tkinter as tk
from tkinter import messagebox


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / ".tools" / "platformio"))

try:
    import serial
except ImportError as exc:
    raise SystemExit(
        "缺少 pyserial。请先运行：py -m pip install pyserial"
    ) from exc


SERIAL_PORT = sys.argv[1].upper() if len(sys.argv) > 1 else "COM4"


class PinEntryApp:
    def __init__(self, root):
        self.root = root
        self.port = None
        self.value = ""
        self.last_connect_attempt = 0.0

        root.title("GR IIIx 蓝牙验证码")
        root.geometry("430x520")
        root.resizable(False, False)
        root.attributes("-topmost", True)
        root.configure(bg="#151515")

        tk.Label(
            root,
            text="GR IIIx 配对验证码",
            font=("Microsoft YaHei UI", 22, "bold"),
            fg="#ff9500",
            bg="#151515",
        ).pack(pady=(18, 5))

        tk.Label(
            root,
            text="相机显示验证码后，直接输入 6 位数字\n第 6 位输入后会自动发送",
            font=("Microsoft YaHei UI", 12),
            fg="white",
            bg="#151515",
        ).pack(pady=(0, 12))

        self.display = tk.Label(
            root,
            text="______",
            font=("Consolas", 42, "bold"),
            fg="#ffffff",
            bg="#252525",
            width=9,
            relief="solid",
            bd=2,
        )
        self.display.pack(pady=8)

        self.status = tk.Label(
            root,
            text=f"正在连接 {SERIAL_PORT}…",
            font=("Microsoft YaHei UI", 11),
            fg="#ffd166",
            bg="#151515",
        )
        self.status.pack(pady=(2, 10))

        tk.Button(
            root,
            text=f"重新连接 {SERIAL_PORT}（F5）",
            command=self.connect_port,
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#3a3a3a",
            fg="white",
            activebackground="#555555",
            activeforeground="white",
            relief="flat",
            padx=16,
            pady=5,
        ).pack(pady=(0, 8))

        keypad = tk.Frame(root, bg="#151515")
        keypad.pack()
        for index, digit in enumerate("123456789"):
            row, col = divmod(index, 3)
            self.make_button(keypad, digit, row, col)
        self.make_button(keypad, "清除", 3, 0, command=self.clear, fg="#ff6b6b")
        self.make_button(keypad, "0", 3, 1)
        self.make_button(keypad, "退格", 3, 2, command=self.backspace, fg="#ffd166")

        root.bind("<Key>", self.on_key)
        root.bind("<F5>", lambda _event: self.connect_port())
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(100, self.connect_port)
        root.after(150, self.poll_serial)
        root.focus_force()

    def make_button(self, parent, label, row, col, command=None, fg="white"):
        if command is None:
            command = lambda d=label: self.add_digit(d)
        tk.Button(
            parent,
            text=label,
            command=command,
            font=("Microsoft YaHei UI", 22, "bold"),
            width=6,
            height=1,
            bg="#303030",
            fg=fg,
            activebackground="#505050",
            activeforeground="white",
            relief="flat",
        ).grid(row=row, column=col, padx=5, pady=5)

    def connect_port(self):
        self.last_connect_attempt = time.monotonic()
        if self.port is not None:
            try:
                self.port.close()
            except Exception:
                pass
            self.port = None
        try:
            port = serial.Serial()
            port.port = SERIAL_PORT
            port.baudrate = 115200
            port.timeout = 0
            # RTS stays inactive so opening the USB CDC port cannot reset the
            # StickS3. DTR is active so Arduino USB CDC accepts host data.
            port.rts = False
            port.dtr = True
            port.open()
            self.port = port
            self.status.config(text=f"{SERIAL_PORT} 已连接 — 等待输入", fg="#55dd88")
        except Exception as exc:
            self.status.config(text=f"{SERIAL_PORT} 连接失败：{exc}", fg="#ff6b6b")

    def add_digit(self, digit):
        if len(self.value) >= 6:
            return
        self.value += digit
        self.refresh_display()
        if len(self.value) == 6:
            self.send_pin()

    def backspace(self):
        self.value = self.value[:-1]
        self.refresh_display()

    def clear(self):
        self.value = ""
        self.refresh_display()
        self.status.config(text="已清除 — 等待输入", fg="#ffd166")

    def refresh_display(self):
        self.display.config(text=self.value + "_" * (6 - len(self.value)))

    def send_pin(self):
        if self.port is None or not self.port.is_open:
            self.connect_port()
            if self.port is None or not self.port.is_open:
                self.status.config(text=f"{SERIAL_PORT} 未连接，验证码未发送", fg="#ff6b6b")
                return
        pin = self.value
        try:
            self.port.write((pin + "\n").encode("ascii"))
            self.port.flush()
            self.status.config(text=f"验证码 {pin} 已发送", fg="#55dd88")
            self.root.bell()
            self.value = ""
            self.root.after(1200, self.refresh_display)
        except Exception as exc:
            self.status.config(text=f"发送失败：{exc}", fg="#ff6b6b")

    def on_key(self, event):
        if event.char and event.char.isdigit() and event.char.isascii():
            self.add_digit(event.char)
        elif event.keysym == "BackSpace":
            self.backspace()
        elif event.keysym == "Escape":
            self.clear()

    def poll_serial(self):
        if self.port is None and (time.monotonic() - self.last_connect_attempt) >= 1.0:
            self.connect_port()
        if self.port is not None and self.port.is_open:
            try:
                waiting = self.port.in_waiting
                if waiting:
                    data = self.port.read(waiting).decode("utf-8", "replace")
                    if "submitted six-digit camera PIN rc=0" in data:
                        self.status.config(text="StickS3 已接受验证码", fg="#55dd88")
                    elif "CAMERA PIN REQUIRED" in data:
                        self.status.config(text="相机已请求验证码 — 请立即输入", fg="#ffd166")
            except Exception as exc:
                self.status.config(text=f"{SERIAL_PORT} 已断开：{exc}", fg="#ff6b6b")
                try:
                    self.port.close()
                except Exception:
                    pass
                self.port = None
        self.root.after(100, self.poll_serial)

    def close(self):
        if self.port is not None:
            try:
                self.port.close()
            except Exception:
                pass
        self.root.destroy()


if __name__ == "__main__":
    window = tk.Tk()
    PinEntryApp(window)
    window.mainloop()
