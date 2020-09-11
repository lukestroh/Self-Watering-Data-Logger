import serial, time
import numpy as np

import onlinesql as ons


port_timeout = 1260 # 21 minutes, in case we water.
HC06_port = "port_name"
ticker = 0

class ReadingDone(Exception):
    pass

# Connect to port, keep trying if it doesn't work
def connectToPort():
    while True:
        try:
            ser = serial.Serial(HC06_port, timeout = port_timeout)
            print("Connected to Bluetooth.")
            return ser
        except serial.serialutil.SerialException:
            print("An error occurred while attempting to connect. Retrying...\n")
        continue


# Update the last unix time data received from the Arduino to txt file.
def updateLastEntry(last_entry):
    with open("last_entry.txt","w", newline='') as last_entry_file:
        last_entry_file.write(last_entry)
        last_entry_file.close()
        time.sleep(0.001)

# Get the last entry, with two returns. One for Python to use to compare, the other to send, with markers, to the Arduino
def getLastEntry():
    with open("last_entry.txt","r") as last_entry_file:
        last_entry = last_entry_file.read()
        last_entry_markers = '<' + last_entry + '>'
        return last_entry, last_entry_markers


# Read from the serial port, save data in an array
def readSerialLines(ser):
    # start a timer for the readline command
    t0 = time.time()
    # read bytes from serial port
    ser_bytes = ser.readline()
    t1 = time.time()
    reading_time = t1-t0
    # if timer exceeds logging time, as it will when we are not connected, close the port and break from loop
    if reading_time >= port_timeout:
        ser.close()        # closing the port also deletes
        raise ReadingDone  # Raise error to break from while loop in getData() so that we can attempt to reconnect to port

    decoded_bytes = (ser_bytes[0:len(ser_bytes)-2].decode("utf-8",errors='replace')) # replaces any errors with a '?'
    decoded_array = np.array(decoded_bytes.split(',')) # Separate my comma-separated string, store in array since the csv writer can only deal with commas this way

    print(decoded_bytes)

    time.sleep(0.01)
    return decoded_array


def getData(ser, database, dbcursor):
    while True:
        try:
            # Send the unix time of the last piece of data we got
            sendData(ser)

            decoded_array = readSerialLines(ser)


            # If the data sent by the Arduino is "False," we know we're going to be updated by the backup. We don't save this line to the file
            if decoded_array[0] == "False":
                # The next data sent is the last unix time stored by the Arudino. This is a reference number and we want to save it in arduino_unix, but we don't want to save to file.
                decoded_array = readSerialLines(ser)

                # Store the next entry, the "char_unix", into a variable
                arduino_unix = decoded_array[0]

                # Print the incoming data from the SD card.
                print("From SD card backup:")
                decoded_array = readSerialLines(ser) # This next line gets the first data line from the SD card. We know it will be the header line, so we will definitely enter the following while loop. We DO NOT want this uploaded to the database.


                while decoded_array[0] != arduino_unix and arduino_unix != '':
                    decoded_array = readSerialLines(ser)

                    # writeToFile(data_file, decoded_array)
                    ons.mySequel.addLine(database, dbcursor, decoded_array)
                    print("check")
                ticker = 1

                print("Live data stream:")

            # If the Arduino had to send old data, then we still have to get the next live data from the Arduino without sending it the old data from 6 min ago using sendData().
            if ticker == 1:
                decoded_array = readSerialLines(ser)
                ticker = 0

            time.sleep(0.01)

            updateLastEntry(decoded_array[0]) # store the latest Unix time value into a text file. That way, we have a 'key' to send the Arduino
            # writeToFile(data_file, decoded_array)

            # Sending data to the database
            ons.mySequel.addLine(database, dbcursor, decoded_array)



        except ReadingDone:
            print("Port Timeout")
            break


# Get the last unix time from your saved file and send to the Arduino.
def sendData(ser):
    last_entry = getLastEntry()
    ser.write(str.encode(last_entry[1]))
    print("Sending data: " + last_entry[0])



"""

Notes:

Something is still seriously wrong with updateLastEntry(), or something in that process. Occasionally, Python doesn't seem to update this, for some unknown reason. Perhaps we need to check the logical flow after the SD card backup?

Watering and then updating the next bit of data seems to take 12 minutes, according to the Arduino time stamp. Why?
8/31/20 ~9:09PM Arduino is watering again?????? (Red LED is lit)
    perhaps the arduino restarted?

    edit: without any noticible restart, the arduino is watering again. Perhaps check the number of files on SD card to examine restarts? ALSO: evidence for restart - "From SD card backup" only send the header line, and no data. This certainly indicates that something is wrong.

    answer: the loop has to wait for its elapsed time. The loop delay is based on the modulo of millis, and with a 10 minute watering time, we have ended up going 4 minutes past what the expected modulo time would be (roughly). This is why we get our data 2 min later.
    What would happen if we watered first, then got data?


"""

def main():
    # Connect to databse
    db = ons.mySequel.connectToDB()
    db.ping(True)
    print(db)
    cursor = db.cursor()

    # Connect to Bluetooth Serial Port
    connection = connectToPort()

    while True:
        try:
            # if the port gets closed, reconnect
            while connection.is_open == False:
                connection = connectToPort()
            getData(connection, db, cursor)
            # if KeyboardInterrupt: # Serial timeout triggers KeyboardInterrupt, making it not possible to include this in code
            #     break
        except:
            continue

if __name__ == "__main__":
    main()
