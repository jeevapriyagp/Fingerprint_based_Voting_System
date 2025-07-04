import serial
import tkinter as tk
from tkinter import messagebox

# Connect to Arduino Serial (change port accordingly)
arduino = serial.Serial(port="COM3", baudrate=9600, timeout=1)  # Change COM3 to your port
waiting_for_finger = False  # Prevents automatic looping

def read_from_arduino():
    global waiting_for_finger
    if arduino.in_waiting > 0:
        line = arduino.readline().decode('utf-8').strip()
        if line:
            print(f"Received: {line}")  # Debugging
            status_label.config(text=line)

            if "New Voter Detected" in line and not waiting_for_finger:
                enroll_voter()
            elif "Voter Found: ID" in line:
                voter_id = line.split()[-1]  # Extract voter ID
                status_label.config(text=f"Voter Found! ID: {voter_id}")
                vote_frame.pack()
                waiting_for_finger = True  # Prevent further processing until next scan
            elif "You have already voted!" in line:
                messagebox.showwarning("Voting Error", "You have already voted!")
                waiting_for_finger = True  # Prevent looping
            elif "Fingerprint not found in database." in line:
                waiting_for_finger = False  # Allow retry for new fingerprint
            elif "Place your finger on the scanner..." in line:
                waiting_for_finger = False  # Reset for next scan

def enroll_voter():
    def save_voter():
        global waiting_for_finger
        name = name_entry.get()
        if name:
            arduino.write((name + "\n").encode())  # Send name to Arduino
            status_label.config(text=f"Registered as: {name}")
            wait_for_fingerprint()
        else:
            messagebox.showwarning("Input Error", "Please enter a name.")

    def wait_for_fingerprint():
        """Wait for 'Place your finger...' message from Arduino before closing."""
        global waiting_for_finger
        while True:
            if arduino.in_waiting > 0:
                response = arduino.readline().decode('utf-8').strip()
                print(f"Received: {response}")  # Debugging
                if "Place your finger..." in response:
                    messagebox.showinfo("Enrollment", "Now place your finger for registration!")
                    enroll_window.destroy()
                    waiting_for_finger = True  # Prevents looping after enrollment failure
                    break

    enroll_window = tk.Toplevel(root)
    enroll_window.title("Enroll New Voter")
    enroll_window.geometry("300x150")
    enroll_window.configure(bg="#f4f4f4")
    center_window(enroll_window, 300, 150)

    tk.Label(enroll_window, text="Enter Name:", font=("Arial", 12), bg="#f4f4f4").pack(pady=5)
    name_entry = tk.Entry(enroll_window, font=("Arial", 12))
    name_entry.pack(pady=5, padx=10)
    tk.Button(enroll_window, text="Submit", command=save_voter, font=("Arial", 12), bg="#4CAF50", fg="white").pack(pady=5)

def cast_vote(choice):
    global waiting_for_finger
    arduino.write((choice + "\n").encode())  # Send vote to Arduino
    messagebox.showinfo("Vote Cast", f"You voted for: {choice}")
    vote_frame.pack_forget()
    waiting_for_finger = True  # Prevents auto-looping

def center_window(win, width, height):
    """Centers the window on the screen."""
    screen_width = win.winfo_screenwidth()
    screen_height = win.winfo_screenheight()
    x = (screen_width // 2) - (width // 2)
    y = (screen_height // 2) - (height // 2)
    win.geometry(f"{width}x{height}+{x}+{y}")

# UI Setup
root = tk.Tk()
root.title("Fingerprint Voting System")
root.geometry("600x500")
root.configure(bg="#e3f2fd")  # Light blue background
center_window(root, 600, 500)

status_label = tk.Label(root, text="Welcome to Voting", font=("Arial", 14, "bold"), fg="#333", bg="#e3f2fd")
status_label.pack(pady=20)

vote_frame = tk.Frame(root, bg="#e3f2fd")
tk.Label(vote_frame, text="Vote for A or B:", font=("Arial", 12, "bold"), bg="#e3f2fd").pack()

btn_a = tk.Button(vote_frame, text="Vote A", command=lambda: cast_vote("A"), font=("Arial", 14, "bold"), bg="#4CAF50", fg="white", width=10)
btn_b = tk.Button(vote_frame, text="Vote B", command=lambda: cast_vote("B"), font=("Arial", 14, "bold"), bg="#f44336", fg="white", width=10)

btn_a.pack(side="left", padx=10, pady=10)
btn_b.pack(side="right", padx=10, pady=10)

# Read Arduino Data Periodically
def update():
    read_from_arduino()
    root.after(1000, update)

update()
root.mainloop()
