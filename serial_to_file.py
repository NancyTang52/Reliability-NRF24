# main code is from https://www.tinkerassist.com/blog/arduino-serial-port-read
import serial.tools.list_ports
import csv

ports = serial.tools.list_ports.comports()
serialInst = serial.Serial()

portsList = []

for onePort in ports:
    portsList.append(str(onePort))
    print(str(onePort))

val = input("Select Port: COM")

for x in range(0,len(portsList)):
    if portsList[x].startswith("COM" + str(val)):
        portVar = "COM" + str(val)
        print(portVar)

serialInst.baudrate = 9600
serialInst.port = portVar
serialInst.open()

file = open("stats.csv", "w", buffering=4096)

try:
    while True:
        try:
            if serialInst.in_waiting:
                packet = serialInst.readline().decode('utf').rstrip('\r\n')
                print(packet)
                if 'end' == packet:
                    break
                elif 'TC_' not in packet:
                    continue
                
                file.write(packet)
                file.write('\n')
                file.flush()
        except UnicodeDecodeError:
            print("Could not decode message")
        except KeyboardInterrupt:
            print("Terminal is closed")
            break
except KeyboardInterrupt:
    print("Terminal is closed")
finally:
    file.close()

file = open("avarage.csv", "w", buffering=4096)

try:
    file.write('Test case;Package Lost;Package send')
    file.write('\n')
    file.flush()
    with open("stats.csv", "r", newline='') as stats:
        csvreader = csv.reader(stats, delimiter=';')
        rows = list(csvreader)
        last_test_case = 'TC_0'
        package_loss_total = 0.0
        amount_of_rows = 0
        last_row_index = len(rows) - 1

        for index, row in enumerate(rows):
            if last_test_case != row[0] or index == last_row_index:
                file.write(f'{last_test_case};{package_loss_total / amount_of_rows};{row[7]}')
                file.write('\n')
                file.flush()
                amount_of_rows = 0
                package_loss_total = 0.0
                last_test_case = row[0]
            amount_of_rows += 1
            package_loss_total += float(row[6])
finally:
    file.close()
