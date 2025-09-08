import tkinter as tk
from tkinter import ttk, messagebox
import threading, queue, time
import serial
from serial.tools import list_ports

BAUD = 115200

ser = None
rx_q = queue.Queue()
tx_q = queue.Queue()
stop_event = threading.Event()
handshake_ok = False

def discover_ports():
    return list_ports.comports()

def open_serial(port):
    global ser, handshake_ok
    close_serial()
    try:
        ser = serial.Serial(port, BAUD, timeout=0.2, write_timeout=0.2)
        # Pro Micro often waits for DTR; assert it
        ser.setDTR(True)
        ser.setRTS(True)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        handshake_ok = False
        return True
    except Exception as e:
        ser = None
        messagebox.showerror("Serial", f"Failed to open {port}:\n{e}")
        return False

def close_serial():
    global ser
    try:
        if ser and ser.is_open:
            ser.close()
    except:
        pass
    ser = None

def reader():
    global handshake_ok
    buf = b""
    while not stop_event.is_set():
        try:
            if ser and ser.is_open:
                n = ser.in_waiting
                if n:
                    buf += ser.read(n)
                    while b"\n" in buf:
                        line, _, buf = buf.partition(b"\n")
                        text = line.decode(errors="ignore").strip()
                        if text == "READY":
                            handshake_ok = True
                            rx_q.put(("status", "READY"))
                        elif text.startswith("STATE "):
                            rx_q.put(("state", text[6:]))
                        else:
                            rx_q.put(("log", text))
                else:
                    time.sleep(0.01)
            else:
                time.sleep(0.05)
        except Exception as e:
            rx_q.put(("log", f"Serial read error: {e}"))
            time.sleep(0.2)

def writer():
    global handshake_ok
    last_motor = None
    while not stop_event.is_set():
        try:
            kind, payload = tx_q.get(timeout=0.1)
        except queue.Empty:
            continue
        if not (ser and ser.is_open):
            continue
        try:
            # Coalesce motor commands (keep only the newest)
            if kind == "motor":
                last_motor = payload
                # drain any queued motors
                try:
                    while True:
                        k2, p2 = tx_q.get_nowait()
                        if k2 == "motor":
                            last_motor = p2
                        else:
                            # send deferred motor after we send this non-motor
                            if last_motor is not None:
                                ser.write(f"MOTOR:{last_motor}\n".encode())
                                last_motor = None
                            ser.write((p2 + "\n").encode())
                except queue.Empty:
                    pass
                if last_motor is not None:
                    ser.write(f"MOTOR:{last_motor}\n".encode())
                    last_motor = None
            else:
                ser.write((payload + "\n").encode())
        except serial.SerialTimeoutException:
            rx_q.put(("log", "Write timeout"))
            time.sleep(0.05)
        except Exception as e:
            rx_q.put(("log", f"Serial write error: {e}"))

# ----------------- GUI -----------------
root = tk.Tk()
root.title("Thermal & Motor Control")

# Top: port selection
top = ttk.Frame(root); top.pack(fill="x", padx=10, pady=8)
ttk.Label(top, text="Port:").pack(side="left")
port_cmb = ttk.Combobox(top, state="readonly", width=35)
def refresh_ports():
    items = []
    for p in discover_ports():
        items.append(f"{p.device} — {p.description}")
    port_cmb["values"] = items
    if items:
        port_cmb.current(0)
refresh_btn = ttk.Button(top, text="Refresh", command=refresh_ports)
connect_btn = ttk.Button(top, text="Connect", command=lambda: do_connect())
port_cmb.pack(side="left", padx=5)
refresh_btn.pack(side="left", padx=5)
connect_btn.pack(side="left", padx=5)

status_var = tk.StringVar(value="Disconnected")
ttk.Label(root, textvariable=status_var, anchor="w").pack(fill="x", padx=10)

# Temps
temps = ttk.LabelFrame(root, text="Temperatures (°C)"); temps.pack(fill="x", padx=10, pady=8)
t0_var = tk.StringVar(value="—"); t1_var = tk.StringVar(value="—"); t2_var = tk.StringVar(value="—"); tds_var = tk.StringVar(value="—")
row = ttk.Frame(temps); row.pack(fill="x", padx=6, pady=6)
for lbl, var in (("A0", t0_var), ("A1", t1_var), ("A2", t2_var), ("DS18B20", tds_var)):
    frame = ttk.Frame(row); frame.pack(side="left", padx=10)
    ttk.Label(frame, text=lbl).pack()
    ttk.Label(frame, textvariable=var, font=("Arial", 12)).pack()

# Controls
ctrl = ttk.LabelFrame(root, text="Controls"); ctrl.pack(fill="x", padx=10, pady=6)

def send_cmd(txt): tx_q.put(("raw", txt))
def set_motor(v):
    try: val = int(float(v))
    except: return
    tx_q.put(("motor", val))

psu_btn = ttk.Button(ctrl, text="PSU ON", command=lambda: send_cmd("PSU_ON"))
psu_off = ttk.Button(ctrl, text="PSU OFF", command=lambda: send_cmd("PSU_OFF"))
heat_on = ttk.Button(ctrl, text="HEATER ON", command=lambda: send_cmd("HEATER_ON"))
heat_off= ttk.Button(ctrl, text="HEATER OFF", command=lambda: send_cmd("HEATER_OFF"))
for b in (psu_btn, psu_off, heat_on, heat_off):
    b.pack(side="left", padx=6, pady=6)

motor_frame = ttk.Frame(ctrl); motor_frame.pack(fill="x", padx=6, pady=6)
ttk.Label(motor_frame, text="Motor PWM").pack(side="left")
motor_scale = ttk.Scale(motor_frame, from_=0, to=255, orient="horizontal", command=set_motor)
motor_scale.set(0)
motor_scale.pack(side="left", fill="x", expand=True, padx=8)

beep_btn = ttk.Button(ctrl, text="Beep 200 ms", command=lambda: send_cmd("BEEP:200"))
beep_btn.pack(side="left", padx=6)

# Log
log = tk.Text(root, height=8, state="disabled"); log.pack(fill="both", expand=True, padx=10, pady=8)

def log_print(s):
    log.configure(state="normal")
    log.insert("end", s + "\n")
    log.see("end")
    log.configure(state="disabled")

def do_connect():
    sel = port_cmb.get().split(" — ")[0] if port_cmb.get() else ""
    if not sel:
        messagebox.showwarning("Serial", "Pick a port first.")
        return
    if open_serial(sel):
        status_var.set(f"Connected {sel} @ {BAUD}, waiting for READY…")
        log_print(f"[INFO] Opened {sel}")
    else:
        status_var.set("Disconnected")

# UI poller for rx_q
def tick():
    try:
        while True:
            kind, payload = rx_q.get_nowait()
            if kind == "status" and payload == "READY":
                status_var.set("Device READY")
                log_print("[INFO] READY")
            elif kind == "state":
                # payload like: "HEATER:0 PSU:1 MOTOR:128 T0:25.1 T1:24.9 T2:24.8 TDS:25.0"
                parts = dict(p.split(":") for p in payload.split())
                t0_var.set(parts.get("T0", "—"))
                t1_var.set(parts.get("T1", "—"))
                t2_var.set(parts.get("T2", "—"))
                tds_var.set(parts.get("TDS", "—"))
            elif kind == "log":
                log_print(payload)
    except queue.Empty:
        pass
    root.after(50, tick)

def on_close():
    stop_event.set()
    root.after(150, lambda: (close_serial(), root.destroy()))

root.protocol("WM_DELETE_WINDOW", on_close)

# Start threads
threading.Thread(target=reader, daemon=True).start()
threading.Thread(target=writer, daemon=True).start()
refresh_ports()
tick()
root.mainloop()
