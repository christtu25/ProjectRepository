import numpy as np
from scipy.spatial.distance import cdist
import gmplot

# Function to calculate geometric median
def geometric_median(X, eps=1e-5):
    y = np.mean(X, axis=0)
   
    while True:
        D = cdist(X, y[np.newaxis, :])
        nonzeros = (D != 0).flatten()
        D_flat = D.flatten()

        if np.sum(nonzeros) == 0:
            return y
       
        Dinv = 1 / D_flat[nonzeros]
        Dinvs = np.sum(Dinv)
        W = Dinv / Dinvs
        T = np.sum(W[:, np.newaxis] * X[nonzeros], axis=0)

        num_zeros = len(X) - np.sum(nonzeros)
        if num_zeros == 0:
            y1 = T
        elif num_zeros == len(X):
            return y
        else:
            R = (T - y) * Dinvs
            r = np.linalg.norm(R)
            rinv = 0 if r == 0 else num_zeros / r
            y1 = max(0, 1 - rinv) * T + min(1, rinv) * y

        if np.linalg.norm(y - y1) < eps:
            return y1

        y = y1

# Function to calculate the center of GPS coordinates and output an HTML file
def calculate_center(beacons):
    # Convert beacons to a NumPy array
    beacons_np = np.array(beacons)
    center = geometric_median(beacons_np)
    
    zoom_level = 18

    # Your Google Maps API key
    gmap = gmplot.GoogleMapPlotter(center[0], center[1], zoom_level, apikey='X')

    # Plot the beacons
    for lat, lon in beacons:
        gmap.marker(lat, lon, 'red')

    # Plot the center
    gmap.marker(center[0], center[1], 'blue')

    # Draw the map
    gmap.draw("map.html")

    print(f"Geometric median (center) is at: {center[0]}, {center[1]}")
    return center

