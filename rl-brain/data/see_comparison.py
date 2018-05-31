import numpy as np
import matplotlib.pyplot as plt

fig, ax = plt.subplots(figsize=(8,4))

scenes = 5

CASE = 'Time to goal (s)'
#CASE = 'Number of collisions'

# SCENES = circle, bottleneck, intersection, bidirectional, hallway, heavyCircle

if CASE == 'Time to goal (s)':
    ORCAmu = (0, 23.91, 18.95, 19.79, 7.72);#, 51.09)
    ORCAsigma = (0, 0.31, 0.12, 0.16, 0.05);#, 0)
    
    SFmu = (16.52, 24.96, 41.29, 33.56, 0);#, 56.53)
    SFsigma = (0.12, 0.41, 2.21, 0.26, 0);#, 0)
    
    rRLmu = (0, 0, 0, 0, 0);#, 0)
    rRLsigma = (0, 0, 0, 0, 0);#, 0)

    mRLpercent = (0, 0, 0, 0, 214/248);#, 0)
    mRLmu = (18.947580645161292, 20.771428571428572, 19.359375, 31.22222222222222, 25.345794392523363);#, 0)
    mRLsigma = (6.189161490313027, 0.337381876531987, 1.360631603107542, 9.475439336372956, 7.151044370152783);#, 0)
    
elif CASE == 'Number of collisions':
    ORCAmu = (0, 3, 0, 0, 2);#, 14)
    ORCAsigma = (0, 0, 0, 0, 0);#, 0)

    SFmu = (8, 9, 19, 21, 0);#, 33)
    SFsigma = (0, 0, 0, 0, 0);#, 0)
    
    rRLmu = (0, 0, 0, 0, 0);#, 0)
    rRLsigma = (0, 0, 0, 0, 0);#, 0)
         
    mRLmu = (5.8596220,  10.747001, 19.359375, 14.2025, 15.50375);#, 0)
    mRLsigma = (3.2676082, 7.8419237, 1.360631603107542, 5.283853, 10.296639);#, 0) 

ind = np.arange(scenes)    # the x locations for the groups
colorList = ['red', 'blue', 'green', 'yellow'];

width = 0.2       # the width of the bars: can also be len(x) sequence

p1 = ax.bar(ind, ORCAmu, width, yerr=ORCAsigma, color="red", alpha=0.5)
p2 = ax.bar(ind+width, SFmu, width, yerr=SFsigma, color="blue", alpha=0.5)
p3 = ax.bar(ind+width*2, rRLmu, width, yerr=rRLsigma, color="green", alpha=0.5)
p4 = ax.bar(ind+width*3, mRLmu, width, yerr=mRLsigma, color="yellow", alpha=0.5)

plt.ylabel(CASE, fontsize=15)
plt.xticks(ind + width*1.5, (r"$\mathtt{circle}$",
                             r"$\mathtt{bottleneck}$",
                             r"$\mathtt{intersection}$",
                             r"$\mathtt{bidirectional}$",
                             r"$\mathtt{hallway}$",
                             r"$\mathtt{heavyCircle}$"),
           rotation=0,
           fontsize=50)
plt.xlim(-0.1,4.9)
plt.tick_params(labelsize=15)

plt.yticks(np.arange(0, 60, 10))
plt.ylim(-1.0,60)
plt.legend((p1[0], p2[0], p3[0], p4[0]), ('ORCA', 'SF', 'regular RL', 'modular RL'), loc='upper left', ncol=2)


ax.annotate('N/A', (0.1, 0.3), ha='center', va='bottom') # set the align
ax.annotate('N/A', (4.3, 0.3), ha='center', va='bottom') # set the alignment of the text

ax.annotate('?', (0.5, 0.3), ha='center', va='bottom') # set the align
ax.annotate('?', (1.5, 0.3), ha='center', va='bottom') # set the align
ax.annotate('?', (2.5, 0.3), ha='center', va='bottom') # set the align
ax.annotate('?', (3.5, 0.3), ha='center', va='bottom') # set the align
ax.annotate('?', (4.5, 0.3), ha='center', va='bottom') # set the align


fig.tight_layout()
plt.show()
