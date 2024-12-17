import time
import serial
import sys
from unittest import TestCase
from datetime import date
from tkinter import *
from tkinter import messagebox
import gc
from tkinter.ttk import Separator, Style

class ProcessOverview():
    
    def __init__(self):
        self.states = ["Processing", "Awaiting", "Received", "Sent", "Failure"]
        self.interaction = False
        self.window = Tk()
        self.window.title('Process overview')
        self.passOnFlag = False
        self.createFlag = False
        self.forwardFlag = False
        self.regCrePassFlag = False
        self.ForwardingTestFlag = False
        self.regAwaDelFlag = False
        self.stoIntFlag = False
        self.pacAlrExiFlag = False
        self.pacNotAvaFlag = False
        self.unkParFlag = False
        self.savLogFlag = False
        self.pollFlag = False
        self.checkFlag = IntVar()
        self.checkFlag.set(1)
        self.Processing_Text = StringVar()
        self.instructionText = StringVar()
        self.State_Text = StringVar()
        self.PackageID = IntVar()
        self.transmitterId = IntVar()
        self.receiverId = IntVar()
        self.f = "Helvetica 14"
        self.instructionText.set("None")
        self.State_Text.set(self.states[0])
        self.Processing_Text.set("0")

        left = Frame(self.window)
        left.pack_propagate(False)
        left.grid(column=0, row = 0, pady=5 ,padx=10, sticky="n")

        right = LabelFrame(self.window,text = "Automatic testing")
        right.pack_propagate(False)
        right.grid(column=3, row = 0, pady=5,padx=10, sticky="n")

        sep1 = Separator(self.window, orient="vertical")
        sep1.grid(column=1, row=0, sticky="ns")

        sep2 = Separator(right, orient="horizontal")
        sep2.grid(column=0, row=1, columnspan = 10, sticky="we")
        
        sep3 = Separator(left, orient="horizontal")
        sep3.grid(column=0, row=3, pady= 12, columnspan = 6, sticky="we")
        #define Labels
        self.l1 = Label(left, text = 'State:', font = self.f)
        self.l1.grid(row = 0, column = 0, sticky = E)

        self.l2 = Label(left, text = 'PackageIp:', font = self.f)
        self.l2.grid(row = 1, column = 0, sticky = E)

        self.l3 = Label(left, text = "Storage:", anchor = W, font = self.f)
        self.l3.grid(row = 2, column = 0, sticky = E)

        self.l4= Label(left, textvariable = self.State_Text, width = 10, font = self.f)
        self.l4.grid(row = 0, column = 1, columnspan = 1)

        self.l5 = Label(left, textvariable = self.Processing_Text, font = self.f)
        self.l5.grid(row = 1, column = 1)

        self.l6 = Label(right,textvariable=self.instructionText, fg = "orange", font = "Helvetica 16 bold")
        self.l6.grid(row = 0, column = 5, columnspan = 2, sticky = N)

        self.list1 = Listbox(left, height = 1, width = 20, font = self.f)
        self.list1.grid(row = 2, column = 1, columnspan = 2, sticky = W)
        self.list1.insert(0," 0, 0, 0, 0, 0, 0")
        
        self.b1 = Button(left, text = "Await / Create Package", width = 20, command = self.packageCreator, font = self.f)
        self.b1.grid(row = 0, column = 3, sticky = E)

        self.b2 = Button(left, text = "Pass on / Deliver Package", width = 20, command = self.packageDeliverer, font = self.f)
        self.b2.grid(row = 1, column = 3, sticky = E)

        self.b3 = Button(left, text= "Help", command=self.show_msg, width = 3, font = self.f)
        self.b3.grid(row = 5, column = 3,sticky = E)

        self.b4 = Button(right,text= "RegularCreate / PassOn", width= 20,command = lambda: self.setRegCrePassFlag(True), font = self.f)
        self.b4.grid(row = 2, column = 5)

        self.b5 = Button(right,text= "RegularAwait / Deliver", width= 20,command = lambda: self.setRegAwDeFlag(True), font = self.f)
        self.b5.grid(row = 3, column = 5)

        self.b6 = Button(right, text="StorageIntegrity", width= 20, command = lambda: self.setStoIntFlag(True), font = self.f)
        self.b6.grid(row = 4, column = 5)

        self.b7 = Button(right, text= "PackageAlreadyExists", width = 20, command = lambda: self.setPacAlrExiFlag(True), font = self.f)
        self.b7.grid(row = 2, column = 6)

        self.b8 = Button(right, text= "PackageNotAvailable", width=  20, command = lambda: self.setPacNotAvaFlag(True), font = self.f)
        self.b8.grid(row = 3, column = 6)

        self.b9 = Button(right, text= "UnkownPartnerId", width = 20, command = lambda: self.setUnkParFlag(True), font = self.f)
        self.b9.grid(row = 4, column = 6)

        self.b10 = Button(right, text= "Save log", width = 10,command = lambda: self.setSavLogFlag(True), font = self.f)
        self.b10.grid(row = 5, column = 6, columnspan= 1, sticky=N)

        self.b11 = Button(left, text = "Poll status", width = 8, command = lambda: self.setPollFlag(True), font = self.f)
        self.b11.grid(row = 4, column = 0)
        
        self.b12 = Button(left, text = "Forward Package", width = 15, command = self.packageForwarder, font = self.f)
        self.b12.grid(row = 2, column = 3, sticky=N)
        
        self.b12 = Button(right, text = "Forwarding", width = 20, command = lambda: self.setForwardingTestFlag(True), font = self.f)
        self.b12.grid(row = 5, column = 5)

        self.c1 = Checkbutton(left, text = "Auto polling",variable = self.checkFlag, onvalue = 1, font = "Helvetica 11")
        self.c1.grid(row = 5, column = 0)


    def poll(self, instruction, answer,storage):
        self.instructionText.set(instruction)
        self.State_Text.set(self.states[int(answer[6])])
        self.Processing_Text.set(str(answer[7]))
        self.list1.insert(0," " + storage)

    def packageCreator(self):
        self.PackageID.set(0)
        self.transmitterId.set(0)
        x = self.window.winfo_rootx()
        y = self.window.winfo_rooty()
        height = self.window.winfo_height()
        geom = "+%d+%d" % (x,y+height)
        self.create_window = Toplevel(self.window)
        self.create_window.title('Package creator')
        self.create_window.wm_geometry(geom)
        self.l1 =Label(self.create_window,text = "Enter package-ID", font = self.f)
        self.l1.grid(row =0, column = 0)
        self.PackageID = IntVar()
        self.e1 = Entry(self.create_window, textvariable= self.PackageID)
        self.e1.grid(row = 1, column = 0)

        self.l2 = Label(self.create_window, text = "Enter partner-ID", font = self.f)
        self.l2.grid(row = 0, column = 1)
        self.e2 = Entry(self.create_window, textvariable= self.transmitterId)
        self.e2.grid(row = 1, column = 1)

        self.b1 = Button(self.create_window, text = "Create / Await", width = 12, command = lambda:self.setCreateFlag(True), font = self.f)
        self.b1.grid(row = 2, column = 1)

    def packageDeliverer(self):
        self.PackageID.set(0)
        self.receiverId.set(0)
        x = self.window.winfo_rootx()
        y = self.window.winfo_rooty()
        height = self.window.winfo_height()
        geom = "+%d+%d" % (x,y+height)
        self.deliver_window = Toplevel(self.window)
        self.deliver_window.title('Pass on package / Deliver package')   
        self.deliver_window.wm_geometry(geom)
        self.l1 =Label(self.deliver_window,text = "Enter package-ID", font = self.f)
        self.l1.grid(row =0, column = 0)
        self.e1 = Entry(self.deliver_window, textvariable= self.PackageID)
        self.e1.grid(row = 1, column = 0)

        self.l2 = Label(self.deliver_window, text = "Enter partner-ID", font = self.f)
        self.l2.grid(row = 0, column = 1)
        self.e2 = Entry(self.deliver_window, textvariable= self.receiverId)
        self.e2.grid(row = 1, column = 1)

        self.b1 = Button(self.deliver_window, text = "Pass on / Deliver", width = 14, command = lambda:self.setPassOnFlag(True), font = self.f)
        self.b1.grid(row = 2, column = 1)

    def packageForwarder(self):
        self.PackageID.set(0)
        self.transmitterId.set(0)
        self.receiverId.set(0)
        x = self.window.winfo_rootx()
        y = self.window.winfo_rooty()
        height = self.window.winfo_height()
        geom = "+%d+%d" % (x,y+height)
        self.forward_window = Toplevel(self.window)
        self.forward_window.title('Package Forwarder')
        self.forward_window.wm_geometry(geom)
        self.l1 =Label(self.forward_window,text = "Enter package-ID", font = self.f)
        self.l1.grid(row =0, column = 0)
        self.PackageID = IntVar()
        self.e1 = Entry(self.forward_window, textvariable= self.PackageID)
        self.e1.grid(row = 1, column = 0)

        self.l2 = Label(self.forward_window, text = "Enter From-ID", font = self.f)
        self.l2.grid(row = 0, column = 1)
        self.e2 = Entry(self.forward_window, textvariable= self.transmitterId)
        self.e2.grid(row = 1, column = 1)        
        
        self.l3 = Label(self.forward_window, text = "Enter To-ID", font = self.f)
        self.l3.grid(row = 0, column = 2)
        self.e3 = Entry(self.forward_window, textvariable= self.receiverId)
        self.e3.grid(row = 1, column = 2)

        self.b1 = Button(self.forward_window, text = "Forward", width = 12, command = lambda:self.setForwardFlag(True), font = self.f)
        self.b1.grid(row = 2, column = 2)

    def onClosing(self):
        if messagebox.askokcancel("Quit", "Do you want to quit?"):
            self.window.destroy()
            return True
        return False

    def show_msg(self):
        messagebox.showinfo('Help',"Make sure to empty the storage befor starting with the test section on the left side.\n\r Every test will clear the storage itself.\n\r Observe the instructions on the upper right side. A new test can be started, if it states `None`")

    def getPassOnData(self, Version, AppID):
        data = [0 ,0, Version, 0, AppID, int(self.PackageID.get()), int(self.receiverId.get()), 3, 2, 1, 0, 99, 000]
        self.deliver_window.destroy()
        return data

    def getCreateData(self, Version,AppID):
        data = [0, 0, Version, 0, AppID, int(self.PackageID.get()), int(self.transmitterId.get()), 3, 2, 1, 0, 99, 000]
        self.create_window.destroy()
        return data
    
    def getForwardData(self, Version, AppID):
        data = [0, 0, Version, 0, AppID, int(self.PackageID.get()), int(self.transmitterId.get()), int(self.receiverId.get()), 2, 1, 0, 99, 000]
        self.forward_window.destroy()
        return data

    def getPassOnFlag(self):
        return self.passOnFlag
    
    def getForwardFlag(self):
        return self.forwardFlag
    
    def setForwardFlag(self, value):
        self.forwardFlag = value
    
    def setPassOnFlag(self, value):
        self.passOnFlag = value

    def getCreatFlag(self):
        return self.createFlag
    
    def setCreateFlag(self, value):
        self.createFlag = value

    def getRegCrePassFlag(self):
        return self.regCrePassFlag
    
    def setRegCrePassFlag(self, value):
        self.regCrePassFlag = value
    
    def setRegAwDeFlag(self,value):
        self.regAwaDelFlag = value
    
    def getRegAwDeFlag(self):
        return self.regAwaDelFlag

    def setStoIntFlag(self, value):
        self.stoIntFlag = value
    
    def getStoIntFlag(self):
        return self.stoIntFlag

    def setPacAlrExiFlag(self, value):
        self.pacAlrExiFlag = value

    def getPacAlrExiFlag(self):
        return self.pacAlrExiFlag

    def setPacNotAvaFlag(self, value):
        self.pacNotAvaFlag = value
    
    def getPacNotAvaFlag(self):
        return self.pacNotAvaFlag

    def setUnkParFlag(self, value):
        self.unkParFlag = value

    def getUnkParFlag(self):
        return self.unkParFlag

    def setSavLogFlag(self, value):
        self.savLogFlag = value
    
    def getSavLogFlag(self):
        return self.savLogFlag
  
    def getCheckFlag(self):
        return self.checkFlag.get()

    def setPollFlag(self, value):
        self.pollFlag = value
    
    def getPollFlag(self):
        return self.pollFlag
    
    def setForwardingTestFlag(self, value):
        self.ForwardingTestFlag = value
    
    def getForwardingTestFlag(self):
        return self.ForwardingTestFlag

class Logger():
    def __init__(self):
        self.log = ""
        self.today = date.today()
        self.d1 = self.today.strftime("%Y%m%d_")
        self.t = time.localtime()
        self.current_time = time.strftime("%H%M%S", self.t)
        self.testAll = bytearray(50)
        self.tested = 0

    def addLog(self, logData, success, testCntFlag):
        self.log += logData
        if testCntFlag == True:
            self.tested += 1
            self.testAll[self.tested-1] =success

    def createLog(self, ID):
        points = 0
        self.log += "\n### Test Done ###\n\n  Result:"
        for x in range(self.tested):
            if(self.testAll[x] == 0):
                self.log  = self.log  + "\n  Test " +str(x+1)+": failed"
            points += self.testAll[x]

        self.log  = self.log  + "\n  Number of tests: " + str(self.tested) + "   passed: "+ str(points) +"\n"
        if points == self.tested:
            self.name = "MMCP_v6_Master_EMU_"+str(self.d1)+str(self.current_time)+"_ID"+str(ID)+"_P.txt"
        else:
            self.name = "MMCP_v6_Master_EMU_"+str(self.d1)+str(self.current_time)+"_ID"+str(ID)+"_F.txt"
        f = open(self.name, "a")
        f.write(self.log)
        f.close()

class MMCP():

    def __init__(self):
        self.Timeout = 1 # wait Time for Broadcast Repeat
        self.success = 0
        self.Version = 6
        self.ID = 0
        self.passOnD = 44
        self.forward = 43
        self.awaitC = 42
        self.poll = 50
        self.storage = ''
        try:
            self.ser = serial.Serial(sys.argv[1],115200, timeout=1) # sys.argv[1]
            if(self.ser.isOpen() == False):
                self.ser.open()
        except Exception as exception:
            print(exception)
            sys.exit(0)

        try:
            self.ID = int(sys.argv[2]) #sys.argv[2]
        except Exception as exception:
            print(exception)
            sys.exit(0)

    def checkMessage(self, data):
        self.success = 1
        if len(self.answer) == 0:
            self.check = "No answer - "
            self.success = 0
        else:

            for x in range(1,14):
                self.message[x-1] = self.answer[x]
            self.check = "\n  Received answer:\n  ["
            for x in range(15):
                self.check  = self.check  + str(int(self.answer[x])) + ", "
            self.check  = self.check  + str(int(self.answer[15])) +"]\n"
            #print(self.check )
            if self.message[4] != data[4]:
                self.check  = "L7_PCI - incorrect"
                self.success = 0
            if self.message[0]!= 0: 
                if self.message[1] != data[1]:
                    self.check  = "Transmitter - incorrect"
                    self.success = 0
                if self.message[3] != data[3]+1:
                    self.check  = "Hops - incorrect"
                    self.success = 0
                if self.message[2] != self.Version:
                    self.check  = "Version - incorrect"
                    self.success = 0
            else:
                if self.message[1] != self.ID:
                    self.check  = "Transmitter - incorrect"
                    self.success = 0
                if self.message[3] != data[3]:
                    self.check  = "Hops - incorrect"
                    self.success = 0
                if self.message[2] != self.Version:
                    self.check  ="Version - incorrect\n"
                    self.success = 0
            if self.answer[14] != self.checkSum(self.message, 13):
                self.check  = "Checksum- incorrect\n"
                self.success = 0
    
    def checkSum(self,message,nBytes):
        data= 0x00
        Inverted = ''
        for byte in range(nBytes):
            data = data+ message[byte]
        data  =  data % 256
        output = bin(data)[2:]
        while len(output)<8:
            output = "0" + output
        for bit in output: 
            if bit == '0':
                Inverted += '1'
            else:
                Inverted += '0'
        return int(Inverted,2)

    def sentPackage(self,data):
        data[0]= self.ID
        rList2 = [0, data[0], data[1], data[2], data[3], data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11], data[12], self.checkSum(bytearray(data), 13),0]
        self.message = bytearray(rList2)
        self.ser.write(self.message)
        self.answer = self.ser.read(16)
        self.checkMessage(data)
    
    def pollStatus(self):
        data = [self.ID, 0, self.Version, 0, self.poll, 0, 91, 3, 2, 1, 0, 99, 000]
        rList2 = [0, data[0], data[1], data[2], data[3], data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11], data[12], self.checkSum(bytearray(data), 13),0]
        self.message = bytearray(rList2)
        self.ser.write(self.message)
        self.answer = self.ser.read(16)
        error_message = self.checkMessage(data)
        if (self.success == 0):
            return error_message
        else:
            self.storage = ""
            for package in self.answer[8:13]:
                self.storage = self.storage + str(package) + ", "
            self.storage = self.storage + str(self.answer[13])
            #print("ID: " + str(self.answer[2]) + ", State: " + str(self.answer[6]) +  ", Package in process: " + str(self.answer[7]) + ", Packages stored: " + self.storage + "\n")

class Tester():
    def __init__(self):
        self.dataLog = ""
        self.instruction = "None"
        self.closingFlag = False
        self.M = MMCP()
        self.W = ProcessOverview()
        self.L = Logger()
        self.main()

    def onClosing(self):
        if self.L.tested > 0: 
            self.L.createLog(self.M.ID)
        if self.W.onClosing() == True: 
            self.closingFlag = True  
        
    def main(self):
        timeCnt = 10000
        initalize = False; 
        while self.closingFlag == False:
            if self.W.getCreatFlag() == True: 
                self.W.setCreateFlag(False)
                data = self.W.getCreateData(self.M.Version, self.M.awaitC)
                self.M.sentPackage(data)

            elif self.W.getPassOnFlag() == True:
                self.W.setPassOnFlag(False)
                data = self.W.getPassOnData(self.M.Version, self.M.passOnD)
                self.M.sentPackage(data)

            elif self.W.getForwardFlag() == True:
                self.W.setForwardFlag(False)
                data = self.W.getForwardData(self.M.Version, self.M.forward)
                self.M.sentPackage(data)
                
            elif self.W.getForwardingTestFlag() == True:
                self.W.setForwardingTestFlag(False)
                self.forwardingTest() 

            elif self.W.getRegCrePassFlag() == True:
                self.W.setRegCrePassFlag(False)
                self.regularCreatePassOn()
            
            elif self.W.getRegAwDeFlag() == True:
                self.W.setRegAwDeFlag(False)
                self.regularAwaitDeliver()

            elif self.W.getStoIntFlag() == True:
                self.W.setStoIntFlag(False)
                self.storageIntegrityTest()

            elif self.W.getPacAlrExiFlag() == True: 
                self.W.setPacAlrExiFlag(False)
                self.packageAlreadyExists()

            elif self.W.getPacNotAvaFlag() == True:
                self.W.setPacNotAvaFlag(False)
                self.packageNotAvailable()
            
            elif self.W.getUnkParFlag() == True: 
                self.W.setUnkParFlag(False)
                self.unknownPartnerId()

            elif self.W.getSavLogFlag() == True: 
                self.W.setSavLogFlag(False)
                if self.L.tested > 0: 
                    self.L.createLog(self.M.ID)
                    self.instruction = "Created log of " + str(self.L.tested) + " test(s)"
                    self.W.poll(self.instruction,self.M.answer, self.M.storage)
                    self.W.window.update()
                    time.sleep(0.5)

                else: 
                    self.instruction = "No data to log yet"
                    self.W.poll(self.instruction,self.M.answer, self.M.storage)
                    self.W.window.update()
                    time.sleep(0.5)

            if (timeCnt >= 1500 and self.W.getCheckFlag() == True) or self.W.getPollFlag() == True or initalize == False:
                self.W.setPollFlag(False) 
                initalize = True
                timeCnt = 0
                self.M.pollStatus()
                self.instruction = "None"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
            else: timeCnt +=1
            
            self.W.window.protocol("WM_DELETE_WINDOW", self.onClosing)
            gc.collect()
            self.W.window.update()

    def polling(self,poll):
        while self.M.answer[6] != poll:
            self.M.pollStatus()
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
    
    def storageClearCheck(self, packageId):
        for space in range(8,14): 
            if self.M.answer[space] == packageId:
                self.instruction = "None"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
                self.W.window.update()
                return False
            else: return True

    def checkForm(self): #Test protocol form 
        if self.M.success == 0: 
            self.instruction = "syntax of received message wrong"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            return False
        return True

    def checkResponse(self, state, package):
        if self.M.answer[6] != state or self.M.answer[7] != package:
            self.instruction = "Response of slave not as expected"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Response of slave not as expected ###\n##### Test failed ######\n\n",0,True)
            return False
        else: return True

    def createPackage(self,packageId):
        self.instruction = "Wait"
        data = [self.M.ID, 0, self.M.Version, 0, self.M.awaitC, packageId, 0, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        time.sleep(0.2)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        self.M.pollStatus()
        
        if not self.checkResponse(1, packageId):
            return False
        
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog(str(self.M.check),1,False)
        time.sleep(0.5)
        #Poll for received message
        self.polling(2)
        time.sleep(0.5)
        #Check if received message is sent more than once 
        self.L.addLog(str(self.M.check),1,False)
        self.M.pollStatus()
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()

        if self.M.answer[6] == 2:
            self.instruction = "Polled state: received twice"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Polled state: received twice ###\n##### Test failed ######\n\n",0,True)
            return False
        
        self.L.addLog(str(self.M.check),1,False)
        self.L.addLog("\n  ### Poll receive part passed ###\n", 0,False)
        return True
  
    def deliverPackage(self, packageId):
        self.instruction = "Wait"
        data = [self.M.ID, 0, self.M.Version, 0, self.M.passOnD, packageId, 0, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(str(self.M.check),1,False)
        time.sleep(0.5)
        self.M.pollStatus()
        if not self.checkResponse(0,packageId):
            return False
        else: 
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),1,False)

            #Poll sent message
            self.polling(3)
            self.L.addLog(str(self.M.check),1,False)
            self.M.pollStatus()
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()

            #Check if sent message is polled more than once
            if self.M.answer[6] == 3:
                self.instruction = "Polled state: sent twice"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
                self.W.window.update()
                time.sleep(0.5)
                self.instruction = "None"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
                self.W.window.update()
                self.L.addLog(str(self.M.check),0,False)
                self.L.addLog("\n  ### Polled state: sent twice ###\n##### Test failed #####\n\n",0,True)
                return False
            elif self.storageClearCheck(packageId) == False:
                self.instruction = "Storage not correctly cleared"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
                self.W.window.update()
                time.sleep(0.5)
                self.instruction = "None"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
                self.W.window.update()
                self.L.addLog(str(self.M.check),0,False)
                self.L.addLog("\n  ### Storage not correctly cleared ###\n##### Test failed #####\n\n",0,True)
                return False

            return True

    def clearStorage(self):
        self.L.addLog("\n  ### Test passed, clearing storage ###\n",1,False)
        storageAddedUp = 100
        while storageAddedUp != 0:
            storageAddedUp = 0
            for space in range(8,14):
                storageAddedUp += self.M.answer[space]
                if self.M.answer[space] != 0:
                    if not self.deliverPackage(self.M.answer[space]):
                        self.instruction = "None"
                        self.W.poll(self.instruction,self.M.answer, self.M.storage)
                        self.W.window.update()
                        return False
        return True

    def regularCreatePassOn(self):
        #Create package 10 
        self.L.addLog("\n##### Test regularCreatePassOn #####\n",1,False)
        self.instruction = "Wait"

        if not self.createPackage(10):
            return

        #Pass on package 10
        time.sleep(0.5)
        data = [self.M.ID, 0, self.M.Version, 0, self.M.passOnD, 10, self.M.ID+1, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        time.sleep(0.3)
        self.W.window.update()
        if not self.checkForm():
            return False
        self.M.pollStatus()

        if not self.checkResponse(0, 10):
            return False

        self.instruction = "Press blue Button"
        self.L.addLog(str(self.M.check)+"\n  Press blue button\n",1,False)
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog(str(self.M.check),1,False)

        #Poll for button press and sent message
        self.polling(3)
        time.sleep(0.5)
        self.L.addLog(str(self.M.check),1,False)
        self.M.pollStatus()
        self.instruction = "Wait"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        
        #Check if sent message is sent more than once
        if self.M.answer[6] == 3:
            self.instruction = "Polled state: sent twice"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.clearStorage()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Polled state: sent twice ###\n##### Test failed #####\n\n",0,True)
            return
        elif self.storageClearCheck(10) == False:
            self.instruction = "Storage not correctly cleared"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.clearStorage()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Storage not correctly cleared ###\n##### Test failed #####\n\n",0,True)
            return
        self.clearStorage()
        self.instruction = "None"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog(self.M.check,1,False)
        self.L.addLog("\n##### Test regularCreatePassOn passed #####\n\n",1,True)

    def regularAwaitDeliver(self):
        #Await package 10 from ID+1 
        self.L.addLog("\n##### Test regularAwaitDeliver #####\n",1,False)
        self.instruction = "Wait"
        data = [self.M.ID, 0, self.M.Version, 0, self.M.awaitC, 10, self.M.ID+1, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        time.sleep(0.5)
        self.M.pollStatus()

        if not self.checkResponse(1, 10):
            return 

        self.instruction = "Press blue Button"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.L.addLog(str(self.M.check)+"\n  Press blue button\n",1,False)

        #Poll for button press and received message
        self.polling(2)
        self.L.addLog(str(self.M.check),1,False)
        self.M.pollStatus()
        self.instruction = "Wait"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()

        #Check if received is polled more than once
        if self.M.answer[6] == 2:
            self.instruction = "Polled state: received twice"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.clearStorage()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog(" \n  ### Polled state: received twice ###\n##### Test failed ######\n\n",0,False)
        else: 
            self.L.addLog(str(self.M.check),1,False)
            self.L.addLog("\n  ### Poll receive part passed ###\n", 0,False)

        #Deliver package 10
        time.sleep(0.5)
        self.deliverPackage(10)
        time.sleep(0.2)
        self.clearStorage()
        self.instruction = "None"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog(self.M.check,1,False)
        self.L.addLog("\n##### Test regularAwaitDeliver passed #####\n\n",1,True)
        #self.L.createLog(self.M.ID)

    def storageIntegrityTest(self):
        packageId = 1
        self.instruction = "Wait"
        #create packages 1 to 6 
        self.L.addLog("\n##### Test StorageIntegrity #####\n",1,False)
        while packageId < 7:
            storageOld = self.M.answer[8:14]
            if not self.createPackage(packageId):
                return
            
            #Check if a new package overwrites an old one 
            writtenFlag = False
            found = 0
            for package in range(8,14):
                for space in range(6):
                    if storageOld[space] == self.M.answer[package] and self.M.answer[package] != 0:
                        found += 1
                    elif self.M.answer[package] == packageId and writtenFlag == False: 
                        writtenFlag = True
                        found += 1
            if found != packageId: 
                self.instruction = "Package has been overwritten"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
                self.W.window.update()
                time.sleep(0.5)
                self.instruction = "None"
                self.W.poll(self.instruction,self.M.answer, self.M.storage)
                self.W.window.update()
                self.L.addLog(str(self.M.check),0,True)
                self.L.addLog("\n  ### One or more packages have been overwritten ###\n##### Test failed ######\n\n",0,False)
                return 

            self.L.addLog(str(self.M.check) + "\n  ### Package " + str(packageId) + " successfully created ###\n",1,False)
            packageId += 1
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
        
        #Check for overflow
        time.sleep(0.5)
        self.L.addLog("\n  ### Overflow check ###\n",0,False)
        storageOld = self.M.answer[8:14]
        data = [self.M.ID, 0, self.M.Version, 0, self.M.awaitC, 7, 0, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        time.sleep(0.2)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        self.M.pollStatus()

        if not self.checkResponse(4, 2):
            return

        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        time.sleep(0.5)
        #Poll for received message
        self.polling(4)
        if self.M.answer[7] == 2: 
            self.L.addLog(str(self.M.check) + "\n  ### Overflow check passed ###\n",1,False)
            
        else:
            self.instruction = "Wrong error message"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Wrong error message ###\n##### Test failed ######\n\n",0,False)
            return 

        self.M.pollStatus()
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        if self.M.answer[6] == 4:
            self.instruction = "Polled state: Failure twice"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Polled state: Failure twice ###\n##### Test failed ######\n\n",0,False)
            return 

        #Deliver the package from the fifth postion
        time.sleep(0.5)
        self.L.addLog("\n  ### Unstore check ###\n",0,False)
        data = [self.M.ID, 0, self.M.Version, 0, self.M.passOnD, int(storageOld[4]), 0, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(str(self.M.check),1,False)
        time.sleep(0.5)
        self.M.pollStatus()

        if not self.checkResponse(0, int(storageOld[4])):
            return

        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.L.addLog(str(self.M.check),1,False)
        self.W.window.update()

        #Poll sent message
        self.polling(3)
        self.L.addLog(str(self.M.check),1,False)
        time.sleep(0.5)
        self.M.pollStatus()
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        
        time.sleep(0.5)
        #Check if sent message is polled more than once
        if self.M.answer[6] == 3:
            self.instruction = "Polled state: sent twice"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Polled state: sent twice ###\n##### Test failed #####\n\n",0,False)
            return 

         #Check if a new packages overwrites an old one 
        clearedFlag = False
        found = 0
        for package in range(8,14):
            for space in range(6):
                if storageOld[space] == self.M.answer[package]:
                    found += 1
                elif self.M.answer[package] == 0 and clearedFlag == False: 
                    clearedFlag = True
                    found += 1
        if found != 6 or clearedFlag == False:
            self.instruction = "Package not correctly cleared"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5) 
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Sent package is not correctly cleared from storage ###\n##### Test failed ######\n\n",0,False)
            return 

        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog(self.M.check,1,False)

        #clear Storage
        if not self.clearStorage():
            return

        self.instruction = "None"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog("\n##### StorageIntegrityTest passed #####\n\n",1,True)

    def packageAlreadyExists(self):
        self.instruction = "Wait"
        self.L.addLog("\n##### Test packageAlreadyExists #####\n",1,False)
        if not self.createPackage(10):
            return

        self.L.addLog("\n  ### Double creation check ###\n",0,False)
        data = [self.M.ID, 0, self.M.Version, 0, self.M.awaitC, 10, 0, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        time.sleep(0.3)
        self.M.pollStatus()

        if not self.checkResponse(4,1):
            return

        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        #Poll for received message
        self.polling(4)
        if self.M.answer[7] == 1: 
            if not self.clearStorage():
                return
            self.L.addLog(str(self.M.check) + "\n##### Test packageAlreadyExists passed #####\n\n",1,True)
            
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
        else:
            if not self.clearStorage():
                return
            self.instruction = "Wrong error message"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Wrong error message ###\n##### Test failed ######\n\n",0,False)

    def packageNotAvailable(self):
        self.instruction = "Wait"
        self.L.addLog("\n##### Test PackageNotAvailable #####\n",1,False)

        data = [self.M.ID, 0, self.M.Version, 0, self.M.passOnD, 10, 0, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        time.sleep(0.3)
        self.M.pollStatus()

        if not self.checkResponse(4, 3):
            return
        
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        time.sleep(0.5)
        #Poll for received message
        self.M.pollStatus()
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        if self.M.answer[6] == 0: 
            self.L.addLog(str(self.M.check), 1, False)
            self.clearStorage()
            self.L.addLog(str(self.M.check), 1, False)
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog("\n##### PackageNotAvailable test passed #####\n\n",1,True)
            
        else:
            self.instruction = "Wrong error message"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Wrong error message ###\n##### Test failed ######\n\n",0,False)

    def unknownPartnerId(self):
        self.instruction = "Wait"
        self.L.addLog("\n##### Test UnknownpartnerId #####\n",1,False)
        self.L.addLog("\n  ### Unknown transmitter id test ###\n",1,False)
        data = [self.M.ID, 0, self.M.Version, 0, self.M.awaitC, 10, self.M.ID+2, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        time.sleep(0.3)
        self.M.pollStatus()

        if not self.checkResponse(4, 4):
            return

        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        time.sleep(0.5)
        #Poll for received message
        self.M.pollStatus()
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        if self.M.answer[6] == 0: 
            self.L.addLog(str(self.M.check) + "\n  ### Unknown transmitter id part passed ###\n",1,False)
        else:
            self.instruction = "Wrong error message"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Wrong error message ###\n##### Test failed ######\n\n",0,False)
            return 

        time.sleep(0.5)
        self.L.addLog("\n  ### Unknown receiver id test ###\n",1, False)
        if not self.createPackage(10):
            return 
        time.sleep(0.5)
        data = [self.M.ID, 0, self.M.Version, 0, self.M.passOnD, 10, self.M.ID+2, 3, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        time.sleep(0.3)
        self.M.pollStatus()

        if not self.checkResponse(4, 4):
            return

        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        time.sleep(0.5)
        #Poll for received message
        self.polling(4)

        time.sleep(0.5)
        if self.M.answer[7] == 4: 
            self.clearStorage()
            self.L.addLog(str(self.M.check) + "\n  ### Unknown receiver id test passed ###\n\n##### UnknownpartnerId test passed #####\n\n",1,True)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
        else:
            self.instruction = "Wrong error message"
            self.clearStorage()
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog("\n  ### Wrong error message ###\n##### Test failed ######\n\n",0,False)
     
    def forwardingTest(self):
        self.instruction = "Wait"
        self.L.addLog("\n##### Test PackageForwarding #####\n",1,False)
        
        data = [self.M.ID, 0, self.M.Version, 0, self.M.forward, 10, self.M.ID-1, self.M.ID+1, 2, 1, 0, 99, 000]
        self.M.sentPackage(data)
        if not self.checkForm():
            return False
        self.L.addLog(self.M.check,1, False)
        time.sleep(0.5)
        self.M.pollStatus()

        if not self.checkResponse(1, 10):
            return 

        self.instruction = "Press blue Button"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.L.addLog(str(self.M.check)+"\n  Press blue button\n",1,False)

        #Poll for button press and received message
        self.polling(2)
        self.L.addLog(str(self.M.check),1,False)
        self.M.pollStatus()
        self.instruction = "Wait"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()

        #Check if package is not stored
        if self.storageClearCheck(10) == False: 
            self.instruction = "Package stored"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Package stored ###\n##### Test failed ######\n\n",0,False)
            return
        
        #Check if received is polled more than once
        if self.M.answer[6] == 2:
            self.instruction = "Polled state: received twice"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.clearStorage()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,True)
            self.L.addLog(" \n  ### Polled state: received twice ###\n##### Test failed ######\n\n",0,False)
        else: 
            self.L.addLog(str(self.M.check),1,False)
            self.L.addLog("\n  ### Receive part passed ###\n", 0,False)
        
        time.sleep(0.5)
        
        
        #Check if correct package id is forwarded
        if self.M.answer[7] != 10:
            self.instruction = "Wrong package id"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Wrong package id ###\n##### Test failed ######\n\n",0,False)
            return
        
        #Check if processing state is reached
        if self.M.answer[6] != 0:
            self.instructions = "Processing state not reached yet"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Processing state not reached yet ###\n##### Test failed ######\n\n",0,False)
            return
        else: 
            self.L.addLog(str(self.M.check),1,False)
            self.L.addLog("\n  ### Processing state reached ###\n", 0,False)
        
        time.sleep(0.5)
               
        #Poll for button press and processing message
        self.instruction = "Press blue Button"
        self.L.addLog(str(self.M.check)+"\n  Press blue button\n",1,False)
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog(str(self.M.check),1,False)

        #Poll for button press and sent message
        self.polling(3)
        time.sleep(0.5)
        self.L.addLog(str(self.M.check),1,False)
        self.M.pollStatus()
        self.instruction = "Wait"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        
        #Cehck if sent message is sent more than once
        if self.M.answer[6] == 3:
            self.instruction = "Polled state: sent twice"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.clearStorage()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Polled state: sent twice ###\n##### Test failed #####\n\n",0,True)
            return
        else:
            self.L.addLog(str(self.M.check),1,False)
            self.L.addLog("\n  ### Sent part passed ###\n", 0,False)
        
        #Check if packageIp is cleared
        if self.M.answer[7] != 0:
            self.instruction = "Package not cleared"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            time.sleep(0.5)
            self.instruction = "None"
            self.W.poll(self.instruction,self.M.answer, self.M.storage)
            self.W.window.update()
            self.L.addLog(str(self.M.check),0,False)
            self.L.addLog("\n  ### Package not cleared ###\n##### Test failed ######\n\n",0,False)
            return
           
        time.sleep(0.5) 
    
        self.instruction = "None"
        self.W.poll(self.instruction,self.M.answer, self.M.storage)
        self.W.window.update()
        self.L.addLog(self.M.check,1,False)
        self.L.addLog("\n##### Test Forwarding passed #####\n\n",1,True)
           
if __name__ == "__main__":
    tester = Tester()
