import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import math
import argparse

plt.close('all')

# parse world propeties
tag = ["goal","collide"];
parser = argparse.ArgumentParser(description='Plot reward for n agents.')
parser.add_argument("-n", "--num_agents", type=int, help='number of agents')
args = parser.parse_args()
NUM_AGENTS = args.num_agents;

# plot properties
TRAIN_PATH = "./reward-punishment.";
TEST_PATH = "./trial.";

REWARD_PATH = [TEST_PATH,];
BIN_SIZE    = [1];

idx_colour = ['r','b','g','y','k','r','b','g','y','k']
idx_legend = []

fig, axes = plt.subplots(nrows=len(tag), ncols=1, figsize=(16, 8), dpi=80, facecolor='w', edgecolor='k')

for goal_tag in tag:
    for _set in range(0,len(REWARD_PATH)):        
        for idx in range(0,NUM_AGENTS):            
            if goal_tag=="goal":
                reward = np.fromfile(REWARD_PATH[_set]+goal_tag+"."+str(idx)+".log",np.float32)[:]
            else:
                reward = -np.fromfile(REWARD_PATH[_set]+goal_tag+"."+str(idx)+".log",np.float32)[:]
            
            T = np.array(range(0,reward.shape[0]-1))
            BIN_NUM = int(math.floor(reward.shape[0]/BIN_SIZE[_set]));
        
            #reward[reward>0]=1;
            
            R_smooth = np.convolve(reward, np.ones(BIN_SIZE[_set])/BIN_SIZE[_set])
            R_smooth = R_smooth[BIN_SIZE[_set]:BIN_SIZE[_set]*BIN_NUM]
            
            im = plt.subplot(len(tag),1,tag.index(goal_tag)+1)

            plt.plot(R_smooth,idx_colour[idx],linewidth=2.5,alpha=0.5)
            
            if goal_tag=="goal":
                plt.ylabel(r'$<$reward$>$', fontsize=20)
                #plt.ylim(-1,30)
            else:
                plt.ylabel('$<$collision$>$', fontsize=20)
                #plt.ylim(-1.0,20.1)
            #plt.xlim(-10,150)
            
        if goal_tag=="goal":
            plt.text(0.9, 0.9, r'goal module', transform=im.transAxes, verticalalignment='top', horizontalalignment='right', fontsize=20)
        else:                
            plt.text(0.95, 0.9, r'collision module', transform=im.transAxes, verticalalignment='top', horizontalalignment='right', fontsize=20)            
            plt.xlabel('time (# of trials)',fontsize=20)
            
print("Number of updates: ",reward.shape[0])        

plt.show()
