import serial
import time
import subprocess
import sys
import os
import threading


def monitor(ss):
    while (True):
        # cur_time = time.time()
        # use_time = cur_time - time_start
        data = ss.read()
        try:
            data = data.decode()
        except UnicodeDecodeError:
            # continue
            data = str(data)
        if not data:
            break
        print(data, end='')
    print("receive finish.")



def ssh_run(fname, usb_port):
    result = {}
    # ---RUN---
    # open serial
    ss = serial.Serial(usb_port, 115200,
                        timeout=1,
                       parity=serial.PARITY_NONE,
                       stopbits=serial.STOPBITS_ONE,
                       bytesize=serial.EIGHTBITS)
    if not ss.isOpen():
        print("OPEN Serial fail")
        return result

    ss.setDTR(False)
    ss.setRTS(True)
    ss.setRTS(False)


    size = os.path.getsize(fname)
    print(f"file size {size}")
    
    time.sleep(0.1)
    # threading.Thread(target=monitor, args=(ss,)).start()
    # ss.write(b"18\r\n")
    # time.sleep(5)
    # return 
    cnt = 0
    while True:
        line = ss.readline().decode()
        print('r ' + line, end='')
        if line == 'start receive data\n':
            break
    with open(fname, 'rb') as f:
        while True:
            count_data = f.read(4)
            if not count_data:
                break
            n = int.from_bytes(count_data, byteorder='little')
            data = f.read(n)
            print(f"send {n} bytes")
            ss.write(count_data)
            ss.write(data)
            # ss.read(n + 4)
            while True:
                line = ss.read(1024)
                try:
                    line = line.decode()[:-1]
                except:
                    line = str(line)
                if not line:
                    break
                print('r ' + line)

    # while (True):
    #     # cur_time = time.time()
    #     # use_time = cur_time - time_start
    #     data = ss.readline()
    #     try:
    #         data = data.decode()
    #     except UnicodeDecodeError:
    #         data = str(data)
    #     # #loge("[k210 autotest]"+str(use_time) + ' ss:',end =' ')
    #     # loge(str(data), end = '')
    #     if len(data) == 0:
    #         break
    #     outf.write(str(data))
    #     outf.flush()
    # outf.close()
    ss.close()
    return result


if __name__ == '__main__':
    arg = '/home/wmj/disk.img.lz4s-8192' if len(sys.argv) <= 1 else sys.argv[1]
    port = '/dev/ttyUSB0' if len(sys.argv) <= 2 else sys.argv[2]
    print(arg, port)

    ssh_run(arg, port)
