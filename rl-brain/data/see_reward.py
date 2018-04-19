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
BIN_SIZE    = [10];

idx_colour = ['r','b','g','y','k','r','b','g','y','k']
idx_legend = []

fig, axes = plt.subplots(nrows=len(tag), ncols=1)

for goal_tag in tag:
    for _set in range(0,len(REWARD_PATH)):        
        for idx in range(0,NUM_AGENTS):            
            if goal_tag=="goal":
                reward = np.fromfile(REWARD_PATH[_set]+goal_tag+"."+str(idx)+".log",np.float32)[0:]
            else:
                reward = -np.fromfile(REWARD_PATH[_set]+goal_tag+"."+str(idx)+".log",np.float32)[0:]
            
            T = np.array(range(0,reward.shape[0]-1))
            BIN_NUM = int(math.floor(reward.shape[0]/BIN_SIZE[_set]));
        
            #reward[reward>0]=1;
            
            R_smooth = np.convolve(reward, np.ones(BIN_SIZE[_set])/BIN_SIZE[_set])
            R_smooth = R_smooth[BIN_SIZE[_set]:BIN_SIZE[_set]*BIN_NUM]
            
            im = plt.subplot(len(tag),1,tag.index(goal_tag)+1)

            plt.plot(R_smooth,idx_colour[idx],linewidth=2.5,alpha=0.5)
            
            if REWARD_PATH[_set]==TRAIN_PATH:
                plt.title('Raw Reward frequency: '+goal_tag)
            elif REWARD_PATH[_set]==TEST_PATH:
                plt.title('Learning curve (evaluation): '+goal_tag)

            plt.xlabel('time (# of trials)')

            if goal_tag=="goal":
                plt.ylabel('reward frequency \n(running average: '+str(BIN_SIZE[_set])+')')
                #plt.ylim(-0.1,1.1)
            else:
                plt.ylabel('collision frequency \n(running average: '+str(BIN_SIZE[_set])+')')
                #plt.ylim(-0.1,4.1)
                
            
        if goal_tag=="goal":
            plt.plot(np.ones(R_smooth.shape[0]),'k--',linewidth=1.5,alpha=1)
        else:                
            plt.plot(np.zeros(R_smooth.shape[0]),'k--',linewidth=1.5,alpha=1)
            
        plt.legend(idx_legend)
        
print("Number of updates: ",reward.shape[0])        

plt.show()
