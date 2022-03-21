import os
import subprocess

os.system("echo ACM Ports found:") 
acm_dispo = subprocess.check_output("ls /dev/ |grep ACM", shell=True)
acm_dispo = acm_dispo.replace(bytes('\n', encoding='utf8'),bytes(' ', encoding='utf8'))
acm_dispo = acm_dispo.replace(bytes('b', encoding='utf8'),bytes('', encoding='utf8'))
acm_dispo = acm_dispo.strip()
list_acm  = acm_dispo.split(bytes(' ', encoding='utf8'))

print(list_acm)

for list_acm in list_acm:
  acm_info = subprocess.check_output("udevadm info -q property -n /dev/" + str(list_acm.decode("utf-8")), shell=True)
  if "ID_PCI_CLASS_FROM_DATABASE=Serial bus controller" not in str(acm_info.decode("utf-8")):
     continue
  else:
  	variable = subprocess.check_output("./usamba /dev/"+ str(list_acm.decode("utf-8") +" read image.bin 0x00003000 32"), shell=True)
  	print(variable) 
   
        



#os.system("udevadm info -q property -n /dev/ttyACM0") 
#os.system("./usamba /dev/ttyACM0 read image.bin 0x00003000 32")
