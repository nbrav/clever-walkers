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
REWARD_SIGN = {"goal":1,"collide":-1}
TRAIN_PATH = "./reward-punishment.";
TEST_PATH = "./trial.";

BIN_SIZE    = 100;

idx_colour = ['r','b','g','y','k','r','b','g','y','k']
idx_legend = []

fig, axes = plt.subplots(nrows=1, ncols=len(tag), figsize=(12, 6), dpi=80, facecolor='w', edgecolor='k')

for goal_tag in tag:
    
    reward_vec = [];
    for idx in range(0,NUM_AGENTS):            

        reward = np.fromfile(TEST_PATH+goal_tag+"."+str(idx)+".log",np.float32)[:] * REWARD_SIGN[goal_tag]
        
        T = np.array(range(0,reward.shape[0]-1))
        BIN_NUM = int(math.floor(reward.shape[0]/BIN_SIZE));
        
        R_smooth = np.convolve(reward, np.ones(BIN_SIZE)/BIN_SIZE)
        R_smooth = R_smooth[BIN_SIZE:BIN_SIZE*BIN_NUM]
    
        reward_vec.append(reward)
        
    im = plt.subplot(1,len(tag),tag.index(goal_tag)+1)

    reward_avg = np.mean(np.asarray(reward_vec),0)
    reward_std = np.std(np.asarray(reward_vec),0)
    plt.plot(reward_avg,idx_colour[idx],linewidth=2.5,alpha=0.5)        
    plt.plot(reward_avg+reward_std,idx_colour[idx],linewidth=1.5,alpha=0.5,color='gray')        
    plt.plot(reward_avg-reward_std,idx_colour[idx],linewidth=1.5,alpha=0.5,color='gray')

    plt.xlabel('time (number of updates)',fontsize=20)
    plt.ylabel(r'$<$'+goal_tag+r'$>$', fontsize=20)
    #plt.ylim(-1,30)
    #plt.xlim(-10,150)        
            
print("Number of updates: ",reward.shape[0])        

plt.show()
