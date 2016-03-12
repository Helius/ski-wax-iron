import time
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import lines
import serial
import re

t_lim = 60 * 10
ax1=plt.axes()  

ax1.set_xticks(np.arange(0,t_lim,30))
ax1.set_yticks(np.arange(0,150,5))

Tdata = [0] * t_lim
line, = plt.plot(Tdata)

Odata = [0] * t_lim
line2, = plt.plot(Odata)

Ddata = [0] * t_lim
line3, = plt.plot(Ddata)

Rdata = [0] * t_lim
line4, = plt.plot(Rdata)

Pdata = [0] * t_lim
line5, = plt.plot(Pdata)

TRdata = [0] * t_lim
line6, = plt.plot(TRdata)


plt.ylim([0,150])

plt.ion()
plt.show()

ser = serial.Serial('/dev/ttyUSB0', 57600)

#adj:0 T1:61.5 T2:0 out:0
t_line = 0
prev_diff = 0
pr = 0
while True:
    inp = ser.readline()
    print 'read:', inp 
    m = re.match(r".*T16C:([-0-9]+).*T2:([0-9]+).*diff:([-0-9]+).*out:([-0-9]+).*pr:([-0-9]+).*u:([0-9]+)", inp)
    if m and m.group(1):
      T1 = float(m.group(1))/16.0
      T2 = int(m.group(2))/16.0
      diff = float(m.group(3))/16.0
      out = float(m.group(4))
      if out > 50:
        out = 50;
      pr = int(m.group(5))
      if pr > 50:
        pr = 0;
      ust = int(m.group(6))
      
      Tdata.append(T1)
      del Tdata[0]
      line.set_xdata(np.arange(len(Tdata)))
      line.set_ydata(Tdata)  # update the data

      Odata.append(out)
      del Odata[0]
      line2.set_xdata(np.arange(len(Odata)))
      line2.set_ydata(Odata)  # update the data
      
      Ddata.append(diff)
      del Ddata[0]
      line3.set_xdata(np.arange(len(Ddata)))
      line3.set_ydata(Ddata)  # update the data
      
      Rdata.append(ust)
      del Rdata[0]
      line4.set_xdata(np.arange(len(Rdata)))
      line4.set_ydata(Rdata)  # update the data
      
      Pdata.append(int(pr)+50)
      del Pdata[0]
      line5.set_xdata(np.arange(len(Pdata)))
      line5.set_ydata(Pdata)  # update the data
      
      TRdata.append(T2)
      del TRdata[0]
      line6.set_xdata(np.arange(len(TRdata)))
      line6.set_ydata(TRdata)  # update the data
      
      plt.grid()
      plt.draw() # update the plot
      plt.grid()
