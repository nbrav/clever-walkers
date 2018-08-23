import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.patches import Wedge 
import numpy as np
import math
import os
import argparse

plt.close('all')

# parse world propeties
parser = argparse.ArgumentParser(description='Plot action rastor for 1-of-n agent.')
parser.add_argument("-a", "--agent_idx", type=int, help='index of the agent')
parser.add_argument("-o", "--objective", type=str, help='name of objective')
args = parser.parse_args()

agent = args.agent_idx; tag = args.objective;

if(tag=="goal"):
    action_size = 36;
else:
    action_size=36;

PATH = "./data/";

plt.figure(figsize=(25,5))

FILE_NAME = PATH+"action."+tag+"."+str(agent)+".log";
atable = np.fromfile(FILE_NAME,np.float32)
    
a = atable.reshape(int(len(atable)/action_size),action_size)
    
T = a.shape[0];

print("agent:"+ str(agent) +" #updates:",a.shape[0], "range:",[a.min(),a.max()])
    
im = plt.subplot(1,1,1)
plt.imshow(a[-500:,:].T,cmap='binary',interpolation="none", aspect='auto')
plt.ylabel('action index', fontsize=25)
plt.xlabel('time', fontsize=25)
plt.title("Action rastor", fontsize=25)
plt.tick_params(axis='both', which='major', labelsize=25)
plt.yticks(np.linspace(0,36,5), ('0', '90', '180', '270'))
#plt.colorbar(orientation="horizontal")

plt.show()
