
import h5py
import numpy as np

U = np.array([2.0,2.0,2.0,2.0,2.0,2.0,2.0,2.0])
t = np.array([[0.0,1.0,0.0,1.0,
               1.0,0.0,0.0,0.0],
              [1.0,0.0,1.0,0.0,
               0.0,1.0,0.0,0.0],
              [0.0,1.0,0.0,1.0,
               0.0,0.0,1.0,0.0],
              [1.0,0.0,1.0,0.0,
               0.0,0.0,0.0,1.0],
              [1.0,0.0,0.0,0.0,
               0.0,1.0,0.0,1.0],
              [0.0,1.0,0.0,0.0,
               1.0,0.0,1.0,0.0],
              [0.0,0.0,1.0,0.0,
               0.0,1.0,0.0,1.0],
              [0.0,0.0,0.0,1.0,
               1.0,0.0,1.0,0.0]])


data = h5py.File("input.h5", "w");

beta = data.create_dataset("BETA", shape=(), dtype='f', data=10.0)

hop_g = data.create_group("hopping")
hop_g.create_dataset("values", data=t)

int_g = data.create_group("interaction")
int_ds = int_g.create_dataset("values", shape=(8,), data=U)

