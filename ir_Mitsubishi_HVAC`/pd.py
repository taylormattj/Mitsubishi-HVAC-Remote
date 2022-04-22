##
## This file is part of the libsigrokdecode project.
##
## Copyright (C) 2022 Matt Taylor <taylormattj@gmail.com>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, see <http://www.gnu.org/licenses/>.
##

import sigrokdecode as srd

class Decoder(srd.Decoder):
    api_version = 3
    id = 'ir_Mitsubishi_HVAC'
    name = 'IR HVAC'
    longname = 'Edge counter'
    desc = 'Count the number of edges in a signal.'
    license = 'gplv2+'
    inputs = ['logic']
    outputs = []
    tags = ['Util']
    channels = (
        {'id': 'data', 'name': 'Data', 'desc': 'Data line'},
    )
    annotations = (
        ('edge_count', 'Edge count'),
        ('bits', 'Bits'),
        ('hex', 'Hex'),
    )
    annotation_rows = (
        ('edge_counts', 'Edges', (0,)),
        ('bits', 'Bits', (1,)),
        ('hex', 'Hex', (2,)),
    )

    def reset(self):
        pass

    def metadata(self, key, value):
        if key == srd.SRD_CONF_SAMPLERATE:
            self.samplerate = value

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def putc(self, cls, ss, annlist):
        self.put(ss, self.samplenum, self.out_ann, [cls, annlist])

    def decode(self):
        edge_count = 0
        edge_start = None
        byte_start = None
        bit_start = 0
        message_start = 0
        bit_count = 0
        new_bit = 2
        bit_0 = 0
        bit_1 = 0
        bit_2 = 0
        bit_3 = 0
        bit_4 = 0
        bit_5 = 0
        bit_6 = 0
        bit_7 = 0
        byte_value = ''

        while True:
            self.wait([{0: 'r'}]) #wait for a rise in the signal
            if edge_start is None: #init the counter
                edge_start = 0
            edge_count += 1
            samples = self.samplenum - edge_start
            t = samples / self.samplerate
            if t > .002:
                self.putc(0, edge_start, ["Header Mark"])
                message_start = 1
            else:
                self.putc(0, edge_start, ["M"])
                bit_start = 1
                if message_start == 2:
                    message_start = 3
                    bit_count = 0
                    byte_start = self.samplenum #restart the edge starting location
                #self.putc(0, edge_start, ["{:d}, {:.3f}ms, 0".format(edge_count, (t * 1000.0))])
            edge_start = self.samplenum #restart the edge starting location
            
            self.wait([{0: 'f'}]) #wait for a fall in the signal
            edge_count += 1
            samples = self.samplenum - edge_start
            t = samples / self.samplerate
            if t > .005:
                self.putc(0, edge_start, ["Repeat"])
            elif t > .0016:
                self.putc(0, edge_start, ["Header Space"])
                if message_start == 1:
                    message_start = 2
                else:
                    message_start = 0
            elif t > .0008:
                #self.putc(0, edge_start, ["1S"])
                if message_start == 3 and bit_start == 1:
                    new_bit = 1
                    bit_start = 0
                else:
                    new_bit = 2
                    bit_start = 0
                self.putc(0, edge_start, ["{:d}, {:d}" .format(new_bit, bit_count)])
            else:
                #self.putc(0, edge_start, ["0S"])
                if message_start == 3 and bit_start == 1:
                    new_bit = 0
                    bit_start = 0
                else:
                    new_bit = 2
                    bit_start = 0
                self.putc(0, edge_start, ["{:d}, {:d}" .format(new_bit, bit_count)])
                #self.putc(0, edge_start, ["{:d}, {:.3f}ms, 1".format(edge_count, (t * 1000.0))])
            if new_bit == 0 or new_bit == 1:
                if bit_count == 0:
                    bit_0 = new_bit
                if bit_count == 1:
                    bit_1 = new_bit
                if bit_count == 2:
                    bit_2 = new_bit
                if bit_count == 3:
                    bit_3 = new_bit
                if bit_count == 4:
                    bit_4 = new_bit
                if bit_count == 5:
                    bit_5 = new_bit
                if bit_count == 6:
                    bit_6 = new_bit
                if bit_count == 7:
                    bit_7 = new_bit
                if bit_count < 7:
                    bit_count += 1
                else:
                    bit_count = 0
                    byte_value = ''
                    if bit_7 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    if bit_6 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    if bit_5 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    if bit_4 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    if bit_3 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    if bit_2 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    if bit_1 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    if bit_0 == 1:
                        byte_value = byte_value + '1'
                    else:
                        byte_value = byte_value + '0'
                    self.putc(1, byte_start, ["{}" .format(byte_value)])
                    decimal_representation = int(byte_value, 2)
                    self.putc(2, byte_start, ["0x{:X}" .format(decimal_representation)])
                    byte_start = self.samplenum #restart the edge starting location
            edge_start = self.samplenum #restart the edge starting location