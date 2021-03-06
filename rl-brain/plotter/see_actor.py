import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.patches import Wedge 
import numpy as np
import math,os,argparse,ipdb
from matplotlib import animation
from matplotlib.patches import Polygon
from matplotlib.collections import PatchCollection

plt.close('all')

# parse world propeties
parser = argparse.ArgumentParser(description='Plot reward for n agents.')
parser.add_argument("-n", "--num_agents", type=int, help='number of agents')
parser.add_argument("-a", "--agent_idx", type=int, help='index of the agent')
parser.add_argument("-o", "--num_objectives", type=str, help='number of objectives')
args = parser.parse_args()
agent = args.agent_idx; NUM_AGENTS = args.num_agents; tag = args.num_objectives;

PATH = "./data/";

# input: dim (num_actions, place-cell X-axis, place-cell Y-axis)    
def create_patches(Q,ax):        

    Xlim = (-1,15); Ylim = (-1,15);
    
    ax1.set_xlim(0,15); ax1.set_ylim(0,15);

    patches = [];
    print("PATCHES:",Q.shape)
    
    num_speed=3;

    print("Q:",np.min(Q),"->",np.max(Q))
    Q = (Q-np.min(Q))/(np.max(Q)-np.min(Q))
    
    for a in range(Q.shape[2]):
        for y in range(Q.shape[1]):
            for x in range(Q.shape[0]):
               
                sector = 90-int(a%36)*10; # action -> angle
                
                speed = np.ceil((a+1)/36);    # action -> speed
                speed = (float(speed)-Xlim[0])/(Xlim[1]-Xlim[0])/num_speed/3.0;
                speed_width = (float(speed)-Xlim[0])/(Xlim[1]-Xlim[0])/num_speed/3.0;
                
                ex = float(x); ey = float(y); # state -> coordinates
                ex = (ex-Xlim[0])/(Xlim[1]-Xlim[0]); ey = (ey-Xlim[0])/(Xlim[1]-Xlim[0]);

                ax.add_artist(Wedge((ex,ey), speed, sector-45/2+2, sector+45/2-2, width=speed_width, transform=ax.transAxes, color=plt.cm.Greys(Q[x,y,a])))
                
    return patches        

if(tag=="goal"):
    state_size_x = 20; state_size_y = 20; action_size = 2 #36*3;
else:
    state_size_x = 50; state_size_y = 20; action_size = 2 #36*3;

fig = plt.figure(figsize=(12,12))

ax1 = plt.subplot(1,1,1)

VIEW_WEIGHTS = False
VIEW_QUIVER = False
VIEW_DYNAMICS = False
VIEW_CONT = True

if VIEW_WEIGHTS:
    
    for agent in range(0,NUM_AGENTS):

        FILE_NAME = PATH+"qvalue."+tag+"."+str(agent)+".log";
        qtable = np.fromfile(FILE_NAME,np.float32)
        
        Q = qtable.reshape(int(len(qtable)/state_size_x/state_size_y/action_size),state_size_x*state_size_y,action_size)
            
        print("agent:"+ str(agent) +" #updates:",Q.shape[0], "sum(Q)=", Q.sum())
        
        im = plt.subplot(NUM_AGENTS,1,agent+1)
        #plt.imshow(Q[-1,:,:].T,cmap='binary',interpolation="none")
        plt.plot(Q[:,:,0])
        plt.xlabel('place cell index'); plt.ylabel('action index');
        plt.title("Agent "+str(agent))

        #plt.colorbar(orientation="horizontal")
        plt.suptitle("Q-values (learning)\nwith "+str(NUM_AGENTS)+" agents")

if VIEW_QUIVER:
    
    FILE_NAME = PATH+"qvalue."+tag+"."+str(agent)+".log";
    qtable = np.fromfile(FILE_NAME,np.float32)
    Q = qtable.reshape(int(len(qtable)/state_size_x/state_size_y/action_size), state_size_x, state_size_y, action_size)
    Q = Q[-1,:,:,:];
    Qmax = np.max(Q); Qmin = np.min(Q);
    
    ax1.set_xticks(()); ax1.set_yticks(());    
    patches = create_patches(Q,ax1);
    
if VIEW_DYNAMICS:
    
    FILE_NAME = PATH+"qvalue."+tag+"."+str(agent)+".log";
    qtable = np.fromfile(FILE_NAME,np.float32)
    
    Q = qtable.reshape(int(len(qtable)/state_size_x/state_size_y/action_size),state_size_x*state_size_y*action_size)

    print("agent:"+ str(agent) +" #updates:",Q.shape[0], "size(Q)=", Q.shape, "sum(Q)=", Q.sum())

    Qavg = np.mean(Q,1)
    Qup = np.std(Q,1);   
    
    im = plt.subplot(1,1,1)
    
    plt.plot(Qavg,linewidth=0.5,alpha=0.5)
    plt.plot(Qavg+Qup,linewidth=1.5,alpha=0.2)
    plt.plot(Qavg-Qup,linewidth=1.5,alpha=0.2)
    plt.xlabel('time'); plt.ylabel('Q');
    plt.title("Agent "+str(agent))
    
    plt.suptitle("Q-values (learning)\nwith "+str(NUM_AGENTS)+" agents")

if VIEW_CONT:
    nSx = 20; nSy = 20; nPI = 4

    FILE_NAME = PATH+"qvalue."+tag+"."+str(agent)+".log";
    actor = np.fromfile(FILE_NAME,np.float32)
    actor = np.reshape(actor,(int(actor.shape[0]/nSx/nSy/nPI),nPI,nSx,nSy))
    plt.imshow(actor[-1,2,:,:].T,cmap='binary',interpolation="none");
    plt.colorbar(); plt.show()

    
plt.show()
