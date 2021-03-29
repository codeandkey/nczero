#!/usr/bin/env python
# Generates the initial model.

import os
import sys
import torch

# Bits per square in input layer.
SQUARE_BITS      = 85

# Number of convolutional filters per Conv2D layer.
RESIDUAL_FILTERS = 16

# Number of residual layers. Will significantly affect size.
RESIDUAL_LAYERS  = 8

path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'latest/network.pt')

if os.path.exists(path):
    print('Model already exists, refusing to overwrite')
    sys.exit(1)

class NCZResidual(torch.nn.Module):
    def __init__(self):
        super(NCZResidual, self).__init__()

        self.conv1 = []
        self.conv2 = []

        for i in range(RESIDUAL_FILTERS):
            self.conv1.append(torch.nn.Conv2d(SQUARE_BITS, SQUARE_BITS, 3, padding=1))
            self.conv2.append(torch.nn.Conv2d(SQUARE_BITS, SQUARE_BITS, 3, padding=1))

        self.bn1 = torch.nn.BatchNorm2d(num_features=SQUARE_BITS)
        self.bn2 = torch.nn.BatchNorm2d(num_features=SQUARE_BITS)
        self.relu1 = torch.nn.ReLU()
        self.relu2 = torch.nn.ReLU()

        # Generate identity layer for skips
        self.skip = torch.nn.Identity()

    def forward(self, x):
        skip_input = x

        for c in self.conv1:
            x = c(x)

        x = self.bn1(x)
        x = self.relu1(x)

        for c in self.conv2:
            x = c(x)

        x = self.bn2(x)
        x = x + self.skip(skip_input)
        x = self.relu2(x)

        return x

class NCZNet(torch.nn.Module):
    def __init__(self):
        super(NCZNet, self).__init__()
        self.conv1 = torch.nn.Conv2d(SQUARE_BITS, SQUARE_BITS, 3, padding=1)
        self.bn1 = torch.nn.BatchNorm2d(num_features=SQUARE_BITS)
        self.relu1 = torch.nn.ReLU()

        # Generate residual layer modules
        self.residuals = []
        for i in range(RESIDUAL_LAYERS):
            self.add_module('residual{0}'.format(i), NCZResidual())

        # Generate value head modules
        self.value_conv1 = torch.nn.Conv2d(SQUARE_BITS, SQUARE_BITS, 1)
        self.value_bn1 = torch.nn.BatchNorm2d(num_features=SQUARE_BITS)
        self.value_relu1 = torch.nn.ReLU()
        self.value_fc1 = torch.nn.Linear(SQUARE_BITS * 64, 256)
        self.value_relu2 = torch.nn.ReLU()
        self.value_fc2 = torch.nn.Linear(256, 1)
        self.value_tanh = torch.nn.Tanh()

        # Generate policy head modules
        self.policy_conv1 = torch.nn.Conv2d(SQUARE_BITS, SQUARE_BITS, 1)
        self.policy_conv2 = torch.nn.Conv2d(SQUARE_BITS, SQUARE_BITS, 1)
        self.policy_bn1 = torch.nn.BatchNorm2d(num_features=SQUARE_BITS)
        self.policy_relu1 = torch.nn.ReLU()
        self.policy_fc1 = torch.nn.Linear(SQUARE_BITS * 64, 4096)
        self.policy_softmax = torch.nn.Softmax(dim=1)

        self.test_bn1 = torch.nn.BatchNorm2d(num_features=SQUARE_BITS)

    def forward(self, inp_board, inp_lmm):
        x = inp_board

        # Apply first convolutional layer
        x = self.conv1(inp_board)
        x = self.bn1(x)
        x = self.relu1(x)

        # Apply residual layers
        for i in self.residuals:
            x = getattr(self, 'residual{0}'.format(i)).forward(x)

        # Policy head
        policy = self.policy_conv1(x)
        policy = self.policy_conv2(policy)
        policy = self.policy_bn1(policy)
        policy = self.policy_relu1(policy)
        policy = torch.reshape(policy, (-1, SQUARE_BITS * 64))
        policy = self.policy_fc1(policy)
        policy = torch.mul(policy, inp_lmm)
        policy = self.policy_softmax(policy)

        # Value head
        value = self.value_conv1(x)
        value = self.value_bn1(value)
        value = self.value_relu1(value)
        value = torch.reshape(value, (-1, SQUARE_BITS * 64))
        value = self.value_fc1(value)
        value = self.value_relu2(value)
        value = self.value_fc2(value)
        value = self.value_tanh(value)

        return (policy, value)

# Initialize model
print('Initializing model. ({} square bits, {} residuals, {} total convs)'.format(
    SQUARE_BITS,
    RESIDUAL_LAYERS,
    RESIDUAL_LAYERS * RESIDUAL_FILTERS * 2 + 1
))

model = NCZNet()

# Test model exec
inp_board = torch.rand((1, SQUARE_BITS, 8, 8))
inp_lmm = torch.ones((1, 4096))

(policy, value) = model(inp_board, inp_lmm)

print ('policy: {}, value: {}'.format(policy.shape, value.shape))

print('Tracing module.')
traced_module = torch.jit.trace(model, (inp_board, inp_lmm))

# Write traced module
traced_module.save(path)
print('Wrote new model to {0}'.format(path))
