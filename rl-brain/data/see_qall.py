import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.patches import Wedge 
import numpy as np
import math

plt.close('all')

def plot_polar():

    num_sec = 24; num_ring = 4; num_relvel = 10; agent=0;
    fig, axes = plt.subplots(nrows=1, ncols=1)
    qtable = np.fromfile("../results/b.collision/qvalue."+tag+"."+str(agent)+".log",np.float32)        
    Q = qtable.reshape(len(qtable)/state_size/action_size,state_size,action_size)
    del qtable

    Q0 = Q[0,:,:].reshape(num_sec+1, num_ring, num_relvel, action_size);    
    #plot_obj = plt.imshow(np.average(np.average(Q0,2),2).T)
    plot_obj = plt.imshow(np.average(Q0,2)[:,:,0].T)
    plot_obj.autoscale();
        
    for t in range(Q.shape[0]):
        Qt = Q[t,:,:].reshape(num_sec+1, num_ring, num_relvel, action_size);
        plot_obj.set_data(np.average(np.average(Qt,2),2).T)
        plot_obj.autoscale();
        plt.draw();        
        plt.pause(0.1);
        plt.title("#epochs:"+str(t))

def plot_timed_q():
    num_sec = 24; num_ring = 4; num_relvel = 10;
    fig, axes = plt.subplots(nrows=1, ncols=1)
    qtable = np.fromfile("../results/b.collision/qvalue."+tag+"."+str(agent)+".log",np.float32)        


    Q = qtable.reshape(len(qtable)/state_size/action_size,(num_sec+1)*num_ring*num_relvel,action_size)
    
    Q_mu = np.average(Q,1)
    Q_std = np.std(Q,1)

    plt.plot(range(Q.shape[0]),Q_mu)
    plt.plot(range(Q.shape[0]),Q_mu-Q_std,'--')
    plt.plot(range(Q.shape[0]),Q_mu+Q_std,'--')
    #plt.fill_between(range(Q.shape[0]), Q_mu-Q_std, Q_mu+Q_std, alpha=0.2, edgecolor='#1B2ACC', facecolor='#089FFF',linewidth=4, linestyle='dashdot', antialiased=True)

agent_num = 4;
tag = "goal"; state_size = 100; action_size = 8*3;
#tag = "collide"; state_size = 1000; action_size=36*5; agent = 0;

#plot_timed_q()
qmin = 0.0; qmax = 10;

for agent in range(0,agent_num):

    FILE_NAME = "qvalue."+tag+"."+str(agent)+".log";
    qtable = np.fromfile(FILE_NAME,np.float32)
    
    #qmax = np.max(qtable); qmin = np.min(qtable);

    if(os.path.isfile(FILE_NAME)==False):
        print "Run the brain first..."
        
    Q = qtable.reshape(len(qtable)/state_size/action_size,state_size,action_size)

    if(Q.shape[0]==0):
        print "Not enough learning.. Wait longer"

    S = range(0,state_size);
    T = Q.shape[0];

    im = plt.subplot(1,agent_num,agent+1)
    plt.imshow(Q[T-1,S,:],cmap='binary')
    plt.ylabel('place cell index')
    plt.xlabel('action index')
    plt.title("Agent "+str(agent))
    plt.clim(qmin,qmax)
    plt.colorbar(orientation="horizontal")

plt.suptitle("Q-values (learning)\nwith "+str(agent_num)+" agents")
plt.show()
