import serial, sys, csv, time
import matplotlib
matplotlib.use("tkAgg")
import matplotlib.pyplot as plt
import numpy as np






def getLastEntry():
    with open("last_entry.txt","r") as last_entry_file:
        last_entry = last_entry_file.read()
        return last_entry

def updateLastEntry(last_entry):
    with open("last_entry.txt","w", newline='') as last_entry_file:
        # lines = last_entry_file.readlines()
        # lines[0] = last_entry
        last_entry_file.write(last_entry)
        last_entry_file.close()


# Declare and try to open port with limited attempts
def connectToPort():
    connect_attempts = 10
    HC06_port = 'COM16'

    # for i in range(0, connect_attempts):
    while True:
        try:
            ser = serial.Serial(HC06_port, baudrate = 9600)
            ser.flushInput()
            return ser
        except serial.serialutil.SerialException:
                # print("An exception occurred while trying to create a serial connection. Trying again... (Attempts left: {number})\n".format(number = connect_attempts - (i+1)))
                print("An exception occurred while trying to create a serial connection. Trying again...\n")
                continue
        break




def sendData(ser):
    last_entry = getLastEntry()
    encoded_last_entry = str.encode(last_entry)
    ser.write(encoded_last_entry)


# # make a plot
# plot_window = 20
# y_var = np.array(np.zeros([plot_window]))
#
# plt.ion()
# fig, ax = plt.subplots()
# line, = ax.plot(y_var)

def getData(ser):
    while True:
        try:
            # while ser.in_waiting:
            ser_bytes = ser.readline()
            decoded_bytes = (ser_bytes[0:len(ser_bytes)-2].decode("utf-8",errors='replace')) # replaces any errors with a '?'
            print(decoded_bytes)
            updateLastEntry(decoded_bytes)

            with open("test_data.csv","a", newline='') as f:
                """
                'a' tells Python to append the serial port data and ensure that no data is erased in the existing file. This prevents overwriting.

                newline='' makes sure we don't have a space between rows


                Note: Opening the .csv file elsewhere will stop the writing process. Perhaps this is why matplotlib isn't working?
                """
                writer = csv.writer(f,delimiter=',')
                writer.writerow([decoded_bytes])
            # y_var = np.append(y_var, decoded_bytes)
            # y_var = y_var[1:plot_window+1]
            # line.set_ydata(y_var)
            # ax.relim()
            # ax.autoscale_view()
            # fig.canvas.draw()
            # fig.canvas.flush_events()

        except:
            print("Keyboard Interrupt")
            break



def userInputter(ser):
    while True:
        while ser.in_waiting:
            ser_bytes = ser.readline()
            decoded_bytes = (ser_bytes[0:len(ser_bytes)-2].decode("utf-8",errors='replace')) # replaces any errors with a '?'
            print(decoded_bytes)
        user_command = input("'1' to turn on LED, '0' to turn in off!")
        usr_command_bytes = str.encode(user_command)
        ser.write(usr_command_bytes)
        time.sleep(0.5)



def main():
    connection = connectToPort()


    # userInputter(connection)



    # sendData(connection)
    # if sendData returns a true, then proceed as normal
    getData(connection)
    # elseif sendData returns a false, get all the next data and save last line

if __name__ == "__main__":
    main()
