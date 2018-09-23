import os
import numpy as np
from scipy import misc
import argparse

''' reads a texture atlas and create a downscaled versions for each tile
    for mipmapping
'''

def parseArguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('texture', help='the atlas texture to split')
    parser.add_argument('width', help='the width of a tile')
    parser.add_argument('height', help='the height of a tile')
    args = parser.parse_args()
    return args

args = parseArguments()

img = misc.imread(args.texture)
nh = h = int(args.height)
nw = w = int(args.width)
level = 0
while (nh > 1 and nw > 1):
    level += 1
    nh /= 2
    nw /= 2
    nimg = np.zeros(((img.shape[0]/w)*nw, (img.shape[1]/h)*nh, img.shape[2]))
    for y in range(0, img.shape[1], h):
        for x in range(0, img.shape[0], w):
            tile = img[x:x+w, y:y+h]
            tile = misc.imresize(tile, (nw, nh), 'bilinear')
            nimg[(x/w)*nw:(x/w+1)*nw, (y/h)*nh:(y/h+1)*nh] = tile
    misc.imsave("%s-%d.png" % (args.texture.split('.')[0], level), nimg)
