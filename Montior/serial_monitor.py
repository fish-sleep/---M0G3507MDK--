"""
龙邱 MSPM0G3507 串口上位机
- 串口调试模式：实时接收并显示串口数据
- 波形显示模式：实时波形图 + PID 参数调节
- 支持 COM 口自动检测、HEX 显示、时间戳
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import datetime
from collections import deque

try:
    import matplotlib
    matplotlib.use("TkAgg")
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    from matplotlib.figure import Figure
    HAS_MPL = True
except ImportError:
    HAS_MPL = False


class SerialMonitor:
    # 波形历史长度
    PLOT_LENGTH = 200

    def __init__(self, root):
        self.root = root
        self.root.title("龙邱 MSPM0G3507 串口上位机")
        self.root.geometry("960x680")
        self.root.minsize(760, 540)

        # ----- 串口对象 -----
        self.ser = None
        self.running = False
        self.read_thread = None

        # ----- 统计数据 -----
        self.byte_count = 0
        self.line_count = 0

        # ----- 波形数据 -----
        self.wave_data1 = deque(maxlen=self.PLOT_LENGTH)
        self.wave_data2 = deque(maxlen=self.PLOT_LENGTH)
        self.wave_x = deque(maxlen=self.PLOT_LENGTH)
        self.wave_counter = 0

        # ----- 数据集录制 -----
        self.recording = False
        self.dataset = []       # [{idx, time, v1, v2, target, err1, err2, pwm1, pwm2}, ...]
        self.record_start_time = 0.0
        self.sample_idx = 0

        # ----- 样式（白色主题）-----
        self.root.configure(bg="#f0f0f0")
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TFrame", background="#f0f0f0")
        style.configure("TLabel", background="#f0f0f0", foreground="#333", font=("Microsoft YaHei UI", 10))
        style.configure("TButton", font=("Microsoft YaHei UI", 10), padding=6)
        style.configure("TCombobox", font=("Microsoft YaHei UI", 10))
        style.configure("TCheckbutton", background="#f0f0f0", foreground="#333", font=("Microsoft YaHei UI", 10))
        style.configure("TNotebook", background="#f0f0f0", borderwidth=0)
        style.configure("TNotebook.Tab", font=("Microsoft YaHei UI", 11), padding=(16, 6))

        self._build_ui()
        self._refresh_ports()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ==================== UI 构建 ====================

    def _build_ui(self):
        # --- 顶部控制栏 ---
        top_frame = ttk.Frame(self.root)
        top_frame.pack(fill=tk.X, padx=12, pady=(12, 6))

        ttk.Label(top_frame, text="COM 口:").pack(side=tk.LEFT)
        self.port_var = tk.StringVar()
        self.port_cb = ttk.Combobox(top_frame, textvariable=self.port_var, width=10, state="readonly")
        self.port_cb.pack(side=tk.LEFT, padx=(4, 12))

        ttk.Label(top_frame, text="波特率:").pack(side=tk.LEFT)
        self.baud_var = tk.StringVar(value="115200")
        baud_cb = ttk.Combobox(top_frame, textvariable=self.baud_var, width=8,
                               values=["9600", "19200", "38400", "57600", "115200", "230400", "256000", "460800", "921600"])
        baud_cb.pack(side=tk.LEFT, padx=(4, 16))

        self.refresh_btn = ttk.Button(top_frame, text="刷新", command=self._refresh_ports)
        self.refresh_btn.pack(side=tk.LEFT, padx=(0, 12))

        self.open_btn = ttk.Button(top_frame, text="打开串口", command=self._toggle_serial)
        self.open_btn.pack(side=tk.LEFT, padx=(0, 12))

        # 状态指示灯
        self.status_canvas = tk.Canvas(top_frame, width=16, height=16, bg="#f0f0f0", highlightthickness=0)
        self.status_canvas.pack(side=tk.LEFT, padx=(0, 8))
        self.status_led = self.status_canvas.create_oval(2, 2, 14, 14, fill="#aaa", outline="")

        self.status_label = ttk.Label(top_frame, text="未连接", foreground="#888")
        self.status_label.pack(side=tk.LEFT)

        # --- 统计条 ---
        stats_frame = ttk.Frame(self.root)
        stats_frame.pack(fill=tk.X, padx=12, pady=(0, 4))
        self.stats_var = tk.StringVar(value="接收字节: 0  |  接收行数: 0  |  运行时间: 00:00:00")
        ttk.Label(stats_frame, textvariable=self.stats_var, foreground="#888",
                  font=("Microsoft YaHei UI", 9)).pack(side=tk.LEFT)

        # --- 模式标签页 ---
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=12, pady=(0, 6))

        self._build_serial_tab()
        self._build_wave_tab()
        self._build_pid_auto_tab()

    def _build_serial_tab(self):
        """串口调试标签页"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="串口调试")

        # --- 选项 ---
        opt_frame = ttk.Frame(tab)
        opt_frame.pack(fill=tk.X, pady=(4, 4))

        self.hex_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(opt_frame, text="HEX 显示", variable=self.hex_var).pack(side=tk.LEFT)

        self.auto_scroll_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(opt_frame, text="自动滚动", variable=self.auto_scroll_var).pack(side=tk.LEFT, padx=(20, 0))

        self.timestamp_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(opt_frame, text="时间戳", variable=self.timestamp_var).pack(side=tk.LEFT, padx=(20, 0))

        self.clear_btn = ttk.Button(opt_frame, text="清空", command=self._clear_output)
        self.clear_btn.pack(side=tk.RIGHT)

        # --- 输出区域 ---
        self.output = scrolledtext.ScrolledText(
            tab, wrap=tk.WORD, font=("Consolas", 11),
            bg="#ffffff", fg="#333333", insertbackground="#333333",
            selectbackground="#bbd6ff", relief=tk.FLAT, borderwidth=1,
            padx=10, pady=10,
        )
        self.output.pack(fill=tk.BOTH, expand=True)

        self.output.tag_configure("hex", foreground="#007acc")
        self.output.tag_configure("ts", foreground="#999")
        self.output.tag_configure("error", foreground="#e03131")
        self.output.tag_configure("warn", foreground="#e88c00")

        # --- 底部发送栏 ---
        send_bar = ttk.Frame(tab)
        send_bar.pack(fill=tk.X, pady=(6, 0))

        ttk.Label(send_bar, text="发送:").pack(side=tk.LEFT, padx=(0, 6))
        self.send_entry = ttk.Entry(send_bar, font=("Microsoft YaHei UI", 10))
        self.send_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 8))
        self.send_entry.bind("<Return>", lambda e: self._send_data())
        self.send_btn = ttk.Button(send_bar, text="发送", command=self._send_data)
        self.send_btn.pack(side=tk.LEFT)
        self.newline_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(send_bar, text="\\r\\n", variable=self.newline_var).pack(side=tk.LEFT, padx=(8, 0))

    def _build_wave_tab(self):
        """波形显示标签页"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="波形显示")

        # --- 上部：波形图 ---
        plot_frame = ttk.Frame(tab)
        plot_frame.pack(fill=tk.BOTH, expand=True, pady=(4, 0))

        self._init_plot(plot_frame)

        # --- 下部：PID 控制面板（三行排列） ---
        pid_container = ttk.Frame(tab)
        pid_container.pack(fill=tk.X, pady=(8, 4))

        # ===== 第1行：速度环 PID =====
        row1 = ttk.Frame(pid_container)
        row1.pack(fill=tk.X, pady=(0, 4))

        ttk.Label(row1, text="速度环", font=("Microsoft YaHei UI", 10, "bold"),
                  width=7, anchor="e").pack(side=tk.LEFT, padx=(0, 10))

        ttk.Label(row1, text="Kp:").pack(side=tk.LEFT)
        self.s_kp_var = tk.StringVar(value="2.0")
        ttk.Entry(row1, textvariable=self.s_kp_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Label(row1, text="Ki:").pack(side=tk.LEFT)
        self.s_ki_var = tk.StringVar(value="0.1")
        ttk.Entry(row1, textvariable=self.s_ki_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Label(row1, text="Kd:").pack(side=tk.LEFT)
        self.s_kd_var = tk.StringVar(value="0.01")
        ttk.Entry(row1, textvariable=self.s_kd_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Button(row1, text="发送速度PID", command=self._send_speed_pid).pack(side=tk.LEFT, padx=(6, 0))

        # ===== 第2行：循迹 PID =====
        row2 = ttk.Frame(pid_container)
        row2.pack(fill=tk.X, pady=(0, 4))

        ttk.Label(row2, text="循迹", font=("Microsoft YaHei UI", 10, "bold"),
                  width=7, anchor="e").pack(side=tk.LEFT, padx=(0, 10))

        ttk.Label(row2, text="Kp:").pack(side=tk.LEFT)
        self.t_kp_var = tk.StringVar(value="1.0")
        ttk.Entry(row2, textvariable=self.t_kp_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Label(row2, text="Ki:").pack(side=tk.LEFT)
        self.t_ki_var = tk.StringVar(value="0.0")
        ttk.Entry(row2, textvariable=self.t_ki_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Label(row2, text="Kd:").pack(side=tk.LEFT)
        self.t_kd_var = tk.StringVar(value="0.0")
        ttk.Entry(row2, textvariable=self.t_kd_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Button(row2, text="发送循迹PID", command=self._send_track_pid).pack(side=tk.LEFT, padx=(6, 0))

        # ===== 第3行：陀螺仪 PID =====
        row3 = ttk.Frame(pid_container)
        row3.pack(fill=tk.X, pady=(0, 4))

        ttk.Label(row3, text="陀螺仪", font=("Microsoft YaHei UI", 10, "bold"),
                  width=7, anchor="e").pack(side=tk.LEFT, padx=(0, 10))

        ttk.Label(row3, text="Kp:").pack(side=tk.LEFT)
        self.g_kp_var = tk.StringVar(value="0.5")
        ttk.Entry(row3, textvariable=self.g_kp_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Label(row3, text="Ki:").pack(side=tk.LEFT)
        self.g_ki_var = tk.StringVar(value="0.0")
        ttk.Entry(row3, textvariable=self.g_ki_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Label(row3, text="Kd:").pack(side=tk.LEFT)
        self.g_kd_var = tk.StringVar(value="0.0")
        ttk.Entry(row3, textvariable=self.g_kd_var, width=7,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(2, 12))

        ttk.Button(row3, text="发送陀螺仪PID", command=self._send_gyro_pid).pack(side=tk.LEFT, padx=(6, 0))

        # ===== 底部工具栏 =====
        tool_row = ttk.Frame(pid_container)
        tool_row.pack(fill=tk.X)

        self.clear_wave_btn = ttk.Button(tool_row, text="清除波形", command=self._clear_wave)
        self.clear_wave_btn.pack(side=tk.LEFT)

        # 录制按钮
        self.record_btn = ttk.Button(tool_row, text="开始录制", command=self._toggle_record)
        self.record_btn.pack(side=tk.LEFT, padx=(12, 0))

        self.record_status = ttk.Label(tool_row, text="样本: 0",
                                       foreground="#888", font=("Consolas", 10))
        self.record_status.pack(side=tk.LEFT, padx=(8, 0))

        self.wave_val_label = ttk.Label(tool_row, text="V1: --  V2: --",
                                        foreground="#888", font=("Consolas", 10))
        self.wave_val_label.pack(side=tk.RIGHT)

    def _init_plot(self, parent):
        """初始化 matplotlib 图表（只放一条线，出现错误也容错）"""
        if not HAS_MPL:
            lbl = ttk.Label(parent, text="未安装 matplotlib，无法显示波形。\n请运行: pip install matplotlib",
                            foreground="#e03131", font=("Microsoft YaHei UI", 12))
            lbl.pack(expand=True)
            return

        self.fig = Figure(figsize=(8, 3.5), dpi=100, facecolor="#ffffff")
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#fafafa")
        self.ax.set_xlabel("采样点")
        self.ax.set_ylabel("数值")
        self.ax.grid(True, linestyle="--", alpha=0.5)

        self.line1, = self.ax.plot([], [], "b-", linewidth=1.5, label="通道1")
        self.line2, = self.ax.plot([], [], "r-", linewidth=1.5, label="通道2")
        self.ax.legend(loc="upper right", fontsize=9)

        self.fig.tight_layout(pad=2)

        self.canvas = FigureCanvasTkAgg(self.fig, master=parent)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    # ==================== 串口操作 ====================

    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if not ports:
            ports = ["无可用串口"]
        self.port_cb["values"] = ports
        if ports and ports[0] != "无可用串口":
            self.port_var.set(ports[0])

    def _toggle_serial(self):
        if self.running:
            self._close_serial()
        else:
            self._open_serial()

    def _open_serial(self):
        port = self.port_var.get()
        if not port or port == "无可用串口":
            messagebox.showwarning("警告", "请先选择有效的 COM 口")
            return

        try:
            baud = int(self.baud_var.get())
        except ValueError:
            messagebox.showwarning("警告", "波特率格式错误")
            return

        try:
            self.ser = serial.Serial(port, baud, timeout=0.1)
        except Exception as e:
            messagebox.showerror("打开失败", f"无法打开 {port}:\n{e}")
            return

        self.running = True
        self.byte_count = 0
        self.line_count = 0
        self.start_time = time.time()

        self.read_thread = threading.Thread(target=self._read_loop, daemon=True)
        self.read_thread.start()

        self.open_btn.config(text="关闭串口")
        self.status_canvas.itemconfig(self.status_led, fill="#2ecc40")
        self.status_label.config(text=f"已连接 {port} @ {baud}", foreground="#2ecc40")
        self._update_stats()

    def _close_serial(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.ser = None

        self.open_btn.config(text="打开串口")
        self.status_canvas.itemconfig(self.status_led, fill="#aaa")
        self.status_label.config(text="未连接", foreground="#888")

    def _read_loop(self):
        buf = b""
        while self.running:
            try:
                if self.ser and self.ser.is_open and self.ser.in_waiting:
                    data = self.ser.read(self.ser.in_waiting)
                    buf += data
                    self.byte_count += len(data)

                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        line = line.rstrip(b"\r")
                        self._process_line(line)
                        self.line_count += 1

                    self.root.after(0, self._update_stats)
            except (serial.SerialException, OSError):
                self.root.after(0, self._close_serial)
                break
            except Exception:
                time.sleep(0.05)

    def _process_line(self, raw: bytes):
        """统一分发：波形数据 → 波形图，其余 → 控制台"""
        if not raw.startswith(b"#"):
            return

        # 去掉帧头 '#'
        body = raw[1:]

        # 尝试解析波形数据: #D,v1,v2
        if body.startswith(b"D,"):
            self._handle_wave_data(body)
            return

        # 其余数据送到串口调试页
        self.root.after(0, self._do_append, body)

    def _handle_wave_data(self, body: bytes):
        """解析波形数据格式: D,v1,v2,target,pwm1,pwm2"""
        try:
            text = body.decode("utf-8")
            parts = text.split(",")
            if len(parts) >= 3:
                v1 = float(parts[1])
                v2 = float(parts[2])
                target = float(parts[3]) if len(parts) >= 4 else 0.0
                pwm1  = float(parts[4]) if len(parts) >= 5 else 0.0
                pwm2  = float(parts[5]) if len(parts) >= 6 else 0.0

                self.wave_counter += 1
                self.wave_x.append(self.wave_counter)
                self.wave_data1.append(v1)
                self.wave_data2.append(v2)

                self.root.after(0, self._update_wave, v1, v2)

                # 录制数据集
                if self.recording:
                    self.sample_idx += 1
                    elapsed = time.time() - self.record_start_time
                    self.dataset.append({
                        "idx":    self.sample_idx,
                        "time":   round(elapsed, 3),
                        "v1":     v1,
                        "v2":     v2,
                        "target": target,
                        "err1":   target - v1,
                        "err2":   target - v2,
                        "pwm1":   pwm1,
                        "pwm2":   pwm2,
                    })
                    # 更新录制采样数
                    self.root.after(0, self._update_record_status)
        except (ValueError, UnicodeDecodeError):
            pass

    def _update_wave(self, v1, v2):
        """在主线程更新波形图"""
        if not HAS_MPL:
            return

        self.wave_val_label.config(text=f"V1: {v1:.2f}  V2: {v2:.2f}")

        x = list(self.wave_x)
        y1 = list(self.wave_data1)
        y2 = list(self.wave_data2)

        if len(x) < 2:
            return

        self.line1.set_data(x, y1)
        self.line2.set_data(x, y2)

        self.ax.relim()
        self.ax.autoscale_view(scalex=True, scaley=True)
        self.canvas.draw_idle()

    # ==================== 串口调试页 ====================

    def _do_append(self, body: bytes):
        output = self.output

        if self.timestamp_var.get():
            ts = datetime.datetime.now().strftime("[%H:%M:%S.%f]")[:-3] + " "
            output.insert(tk.END, ts, "ts")

        if self.hex_var.get():
            hex_str = " ".join(f"{b:02X}" for b in body) + "\n"
            output.insert(tk.END, hex_str, "hex")
        else:
            try:
                text = body.decode("utf-8")
            except UnicodeDecodeError:
                try:
                    text = body.decode("gbk")
                except UnicodeDecodeError:
                    text = body.decode("utf-8", errors="replace")
            output.insert(tk.END, text + "\n")

        if self.auto_scroll_var.get():
            output.see(tk.END)

    def _send_data(self):
        """从串口调试页手动发送"""
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("警告", "请先打开串口")
            return

        text = self.send_entry.get()
        if not text:
            return

        try:
            data = text.encode("utf-8")
            if self.newline_var.get():
                data += b"\r\n"
            self.ser.write(data)
            self.send_entry.delete(0, tk.END)
        except Exception as e:
            messagebox.showerror("发送失败", str(e))

    def _clear_output(self):
        self.output.delete("1.0", tk.END)
        self.byte_count = 0
        self.line_count = 0
        self.start_time = time.time()
        self._update_stats()

    def _update_stats(self):
        elapsed = int(time.time() - self.start_time) if self.running else 0
        h, m, s = elapsed // 3600, (elapsed % 3600) // 60, elapsed % 60
        self.stats_var.set(
            f"接收字节: {self.byte_count}  |  接收行数: {self.line_count}  |  运行时间: {h:02d}:{m:02d}:{s:02d}"
        )
        if self.running:
            self.root.after(500, self._update_stats)

    # ==================== 波形页：PID 控制 ====================

    def _send_speed_pid(self):
        """发送速度环 PID: #S,Kp,Ki,Kd!"""
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("警告", "请先打开串口")
            return
        try:
            cmd = f"#S,{self.s_kp_var.get()},{self.s_ki_var.get()},{self.s_kd_var.get()}!"
            self.ser.write(cmd.encode("utf-8"))
            self._show_sent(cmd)
        except Exception as e:
            messagebox.showerror("发送失败", str(e))

    def _send_track_pid(self):
        """发送循迹 PID: #T,Kp,Ki,Kd!"""
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("警告", "请先打开串口")
            return
        try:
            cmd = f"#T,{self.t_kp_var.get()},{self.t_ki_var.get()},{self.t_kd_var.get()}!"
            self.ser.write(cmd.encode("utf-8"))
            self._show_sent(cmd)
        except Exception as e:
            messagebox.showerror("发送失败", str(e))

    def _send_gyro_pid(self):
        """发送陀螺仪 PID: #G,Kp,Ki,Kd!"""
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("警告", "请先打开串口")
            return
        try:
            cmd = f"#G,{self.g_kp_var.get()},{self.g_ki_var.get()},{self.g_kd_var.get()}!"
            self.ser.write(cmd.encode("utf-8"))
            self._show_sent(cmd)
        except Exception as e:
            messagebox.showerror("发送失败", str(e))

    def _show_sent(self, text):
        """将发送的指令回显到串口调试页"""
        output = self.output
        ts = datetime.datetime.now().strftime("[%H:%M:%S.%f]")[:-3] + " "
        output.insert(tk.END, ts + "TX → ", "ts")
        output.insert(tk.END, text + "\n", "warn")
        output.see(tk.END)

    # ==================== PID 自整定标签页 ====================

    def _build_pid_auto_tab(self):
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="PID自整定")

        # --- 当前 PID ---
        top = ttk.Frame(tab)
        top.pack(fill=tk.X, padx=12, pady=(8, 4))

        ttk.Label(top, text="当前PID:", font=("Microsoft YaHei UI", 10, "bold")).pack(side=tk.LEFT)
        self.cur_pid_var = tk.StringVar(value="Kp=5.0  Ki=0.0  Kd=0.5")
        ttk.Label(top, textvariable=self.cur_pid_var,
                  foreground="#007acc", font=("Consolas", 11)).pack(side=tk.LEFT, padx=(8, 20))

        ttk.Label(top, text="目标速度:").pack(side=tk.LEFT)
        self.target_var = tk.StringVar(value="50")
        ttk.Entry(top, textvariable=self.target_var, width=6,
                  font=("Microsoft YaHei UI", 10)).pack(side=tk.LEFT, padx=(4, 0))
        ttk.Label(top, text="mm/s").pack(side=tk.LEFT, padx=(2, 12))

        self.auto_send_btn = ttk.Button(top, text="发送推荐PID", command=self._send_recommended_pid)
        self.auto_send_btn.pack(side=tk.LEFT)

        ttk.Button(top, text="清空数据集", command=self._clear_dataset).pack(side=tk.LEFT, padx=(8, 0))

        # --- 数据集表格 ---
        tree_frame = ttk.Frame(tab)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=12, pady=4)

        columns = ("样本", "时间(s)", "V1", "V2", "目标", "误差1", "误差2", "PWM1", "PWM2")
        self.tree = ttk.Treeview(tree_frame, columns=columns, show="headings",
                                 height=12, selectmode="extended")

        col_widths = [50, 70, 75, 75, 65, 75, 75, 65, 65]
        for col, w in zip(columns, col_widths):
            self.tree.heading(col, text=col, anchor="center")
            self.tree.column(col, width=w, anchor="e", minwidth=w - 10)

        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # --- 分析结果 ---
        result_frame = ttk.Frame(tab)
        result_frame.pack(fill=tk.X, padx=12, pady=(4, 8))

        self.analysis_text = scrolledtext.ScrolledText(
            result_frame, height=8, font=("Consolas", 10),
            bg="#fafafa", fg="#333", relief=tk.FLAT, borderwidth=1,
            padx=8, pady=6, wrap=tk.WORD
        )
        self.analysis_text.pack(fill=tk.BOTH, expand=True)

        self.analysis_text.tag_configure("good", foreground="#2ecc40")
        self.analysis_text.tag_configure("warn", foreground="#e88c00")
        self.analysis_text.tag_configure("header", foreground="#007acc", font=("Consolas", 11, "bold"))

    # ==================== 录制控制 ====================

    def _toggle_record(self):
        if not self.recording:
            self._start_record()
        else:
            self._stop_record()

    def _start_record(self):
        self.dataset.clear()
        self.sample_idx = 0
        self.recording = True
        self.record_start_time = time.time()
        self.record_btn.config(text="停止录制")
        self.record_status.config(text="样本: 0", foreground="#e03131")

        # 清空表格
        for item in self.tree.get_children():
            self.tree.delete(item)

    def _stop_record(self):
        self.recording = False
        self.record_btn.config(text="开始录制")
        self.record_status.config(text=f"样本: {len(self.dataset)}", foreground="#888")
        self._populate_table()
        self._analyze_and_suggest()

    def _update_record_status(self):
        self.record_status.config(text=f"样本: {len(self.dataset)}", foreground="#e03131")

    def _populate_table(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        for d in self.dataset:
            self.tree.insert("", tk.END, values=(
                d["idx"],
                f"{d['time']:.3f}",
                f"{d['v1']:.1f}",
                f"{d['v2']:.1f}",
                f"{d['target']:.1f}",
                f"{d['err1']:.1f}",
                f"{d['err2']:.1f}",
                f"{d['pwm1']:.0f}",
                f"{d['pwm2']:.0f}",
            ))

    def _clear_dataset(self):
        self.dataset.clear()
        self.sample_idx = 0
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.analysis_text.delete("1.0", tk.END)

    # ==================== PID 分析 ====================

    def _analyze_and_suggest(self):
        if len(self.dataset) < 20:
            self.analysis_text.delete("1.0", tk.END)
            self.analysis_text.insert(tk.END, "数据太少，请录制至少 2 秒（约 200 个样本）\n", "warn")
            return

        # 分离两路通道
        speeds1 = [d["v1"] for d in self.dataset]
        speeds2 = [d["v2"] for d in self.dataset]
        errors1 = [d["err1"] for d in self.dataset]
        errors2 = [d["err2"] for d in self.dataset]
        pwms1   = [d["pwm1"] for d in self.dataset]
        pwms2   = [d["pwm2"] for d in self.dataset]
        target  = self.dataset[0]["target"]
        if target == 0:
            target = 50.0

        def analyze(ch_name, speeds, errors, pwms, target):
            n = len(speeds)
            if n < 20:
                return None, f"[{ch_name}] 样本不足\n"

            max_speed = max(speeds)
            min_speed = min(speeds[n // 2:])  # 后半段最低点（跳过初始加速）
            overshoot = max(0, max_speed - target)
            overshoot_pct = overshoot / target * 100 if target > 0 else 0

            # 达到目标 ±5% 所需的样本数 (settling)
            settled_idx = n
            for i in range(n // 2, n):
                if abs(speeds[i] - target) < target * 0.05:
                    settled_idx = i
                    break
            settle_time = settled_idx * 0.01  # 10ms 采样

            # 稳态误差（后 30% 数据平均值与目标的差）
            steady_start = int(n * 0.7)
            steady_speeds = speeds[steady_start:]
            steady_avg = sum(steady_speeds) / len(steady_speeds) if steady_speeds else 0
            steady_err = steady_avg - target

            # RMS 误差
            rms = (sum(e ** 2 for e in errors) / n) ** 0.5

            # 振荡判断（穿越目标值次数）
            cross_count = 0
            above = speeds[n // 4] > target
            for s in speeds[n // 4:]:
                if (s > target) != above:
                    cross_count += 1
                    above = s > target

            avg_pwm = sum(pwms[n // 2:]) / max(n - n // 2, 1)

            metrics = {
                "overshoot": overshoot, "overshoot_pct": overshoot_pct,
                "settle_time": settle_time, "steady_err": steady_err,
                "rms": rms, "cross_count": cross_count,
                "avg_pwm": avg_pwm, "max_speed": max_speed,
                "target": target,
            }

            text = (
                f"[{ch_name}]\n"
                f"  超调量:    {overshoot:.1f} mm/s  ({overshoot_pct:.1f}%)\n"
                f"  调节时间:  {settle_time:.2f}s\n"
                f"  稳态误差:  {steady_err:.1f} mm/s\n"
                f"  RMS 误差:  {rms:.1f}\n"
                f"  振荡次数:  {cross_count} 次\n"
                f"  平均PWM:   {avg_pwm:.0f}\n"
            )
            return metrics, text

        metrics1, text1 = analyze("通道1(M1)", speeds1, errors1, pwms1, target)
        metrics2, text2 = analyze("通道2(M2)", speeds2, errors2, pwms2, target)

        # 计算推荐 PID
        new_kp, new_ki, new_kd = self._calc_new_pid(
            (5.0, 0.0, 0.5), metrics1, metrics2, target)

        suggestion = (
            f"\n{'─'*40}\n"
            f"推荐 PID:  Kp={new_kp:.2f}  Ki={new_ki:.2f}  Kd={new_kd:.2f}\n"
            f"(当前:      Kp=5.00  Ki=0.00  Kd=0.50)\n"
            f"点击「发送推荐PID」将新值写入 MCU\n"
        )

        self.analysis_text.delete("1.0", tk.END)
        if text1:
            self.analysis_text.insert(tk.END, text1)
        if text2:
            self.analysis_text.insert(tk.END, "\n")
            self.analysis_text.insert(tk.END, text2)
        self.analysis_text.insert(tk.END, suggestion, "header")

        # 保存推荐值供发送使用
        self.recommended_pid = (new_kp, new_ki, new_kd)

    def _calc_new_pid(self, current, m1, m2, target):
        """根据性能指标计算新的 PID"""
        Kp, Ki, Kd = current

        if m1 is None or m2 is None:
            return Kp, Ki, Kd

        # 取两通道平均值
        overshoot = (m1["overshoot_pct"] + m2["overshoot_pct"]) / 2
        steady_err = abs((m1["steady_err"] + m2["steady_err"]) / 2)
        rms = (m1["rms"] + m2["rms"]) / 2
        cross_count = (m1["cross_count"] + m2["cross_count"]) / 2

        # --- 调 Kp ---
        if overshoot > 25:
            Kp *= 0.75        # 超调过大，大幅降低 Kp
        elif overshoot > 15:
            Kp *= 0.88
        elif overshoot < 3 and m1["settle_time"] > 1.0:
            Kp *= 1.12        # 超调太小但响应慢，提高 Kp
        elif overshoot < 3:
            Kp *= 1.05

        # --- 调 Kd ---
        if cross_count > 5:
            Kd += 0.3         # 振荡多，加大 D 抑制
        elif cross_count > 2:
            Kd += 0.15
        elif cross_count == 0 and overshoot > 10:
            Kd += 0.1         # 超调但没振荡，轻微加 D

        # --- 调 Ki ---
        if abs(steady_err) > 5:
            Ki += 0.03        # 稳态误差大，加积分
        elif abs(steady_err) > 2:
            Ki += 0.015

        # 限幅
        Kp = max(0.5, min(Kp, 50.0))
        Ki = max(0.0, min(Ki, 5.0))
        Kd = max(0.0, min(Kd, 10.0))

        return round(Kp, 2), round(Ki, 2), round(Kd, 2)

    def _send_recommended_pid(self):
        if not hasattr(self, 'recommended_pid'):
            messagebox.showwarning("警告", "请先录制数据")
            return
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("警告", "请先打开串口")
            return

        kp, ki, kd = self.recommended_pid
        cmd = f"#S,{kp:.2f},{ki:.2f},{kd:.2f}!"
        try:
            self.ser.write(cmd.encode("utf-8"))
            self._show_sent(cmd)
        except Exception as e:
            messagebox.showerror("发送失败", str(e))

    # ==================== 波形控制 ====================

    def _clear_wave(self):
        self.wave_data1.clear()
        self.wave_data2.clear()
        self.wave_x.clear()
        self.wave_counter = 0
        if HAS_MPL:
            self.line1.set_data([], [])
            self.line2.set_data([], [])
            self.ax.relim()
            self.ax.autoscale_view()
            self.canvas.draw_idle()
        self.wave_val_label.config(text="V1: --  V2: --")

    # ==================== 关闭 ====================

    def _on_close(self):
        self._close_serial()
        self.root.destroy()


def main():
    root = tk.Tk()
    SerialMonitor(root)
    root.mainloop()


if __name__ == "__main__":
    main()
