import tkinter as tk
from tkinter import ttk
import serial
import threading

# Define serial port settings
SERIAL_PORT = 'COM13'  # Replace with your actual port
BAUD_RATE = 9600

# Serial connection object
ser = None

# Try to connect serial
def connect_serial():
    global ser
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    except Exception as e:
        print(f"Error connecting to serial: {e}")

# Send data to Arduino
def send_command(cmd):
    if ser and ser.is_open:
        ser.write(f"{cmd}\n".encode())

# Thread-safe temperature update
def read_serial():
    while True:
        if ser and ser.in_waiting:
            try:
                line = ser.readline().decode().strip()
                if line.startswith("TEMP(C):"):
                    temp = line.split(":")[1].strip()
                    temp_var.set(temp)
            except:
                pass

# Create GUI window
root = tk.Tk()
root.title("Thermal & Motor Control Interface")

# Temperature display
temp_frame = ttk.LabelFrame(root, text="Temperature Sensor")
temp_frame.pack(fill="x", padx=10, pady=5)

temp_var = tk.StringVar(value="N/A")
ttk.Label(temp_frame, text="Temperature (°C):").pack(side="left", padx=5)
ttk.Label(temp_frame, textvariable=temp_var, font=("Arial", 12)).pack(side="left")

# Heater control
heater_frame = ttk.LabelFrame(root, text="Heater Control")
heater_frame.pack(fill="x", padx=10, pady=5)

def toggle_heater():
    if heater_btn.config('text')[-1] == 'Turn ON':
        send_command("HEATER_ON")
        heater_btn.config(text='Turn OFF')
    else:
        send_command("HEATER_OFF")
        heater_btn.config(text='Turn ON')

heater_btn = ttk.Button(heater_frame, text="Turn ON", command=toggle_heater)
heater_btn.pack(padx=10, pady=5)

# Motor speed control (Speaker PWM)
motor_frame = ttk.LabelFrame(root, text="Speaker PWM Control")
motor_frame.pack(fill="x", padx=10, pady=5)

def set_motor_speed(val):
    send_command(f"MOTOR:{val}")

speed_scale = ttk.Scale(motor_frame, from_=0, to=255, orient="horizontal", command=lambda v: set_motor_speed(int(float(v))))
speed_scale.set(0)
speed_scale.pack(fill="x", padx=10)

# Stepper motor control
stepper_frame = ttk.LabelFrame(root, text="Stepper Motor Control")
stepper_frame.pack(fill="x", padx=10, pady=5)

def send_steps():
    try:
        count = int(step_entry.get())
        send_command(f"STEP:{count}")
    except ValueError:
        print("Invalid step count")

step_entry = ttk.Entry(stepper_frame, width=10)
step_entry.insert(0, "100")  # default
step_entry.pack(side="left", padx=5)

step_btn = ttk.Button(stepper_frame, text="Send Steps", command=send_steps)
step_btn.pack(side="left", padx=5)

# Start everything
connect_serial()
threading.Thread(target=read_serial, daemon=True).start()
root.mainloop()
