import numpy as np


def char_to_bin(message):
    ret_array = [0, 0, 0]
    bit_array = np.uint32(0)
    bit_position = 0

    for char in message:
        if char == '1':
            bit_array |= 1<<(bit_position)
        bit_position += 1

    ret_array[0] = (bit_array & 0x000000FF)
    ret_array[1] = (bit_array & 0x0000FF00) >> 8
    ret_array[2] = (bit_array & 0x00FF0000) >> 16

    out_message = chr(ret_array[0])+chr(ret_array[1])+chr(ret_array[2])
    return out_message


def bin_to_char(message):
    char_array = ''
    for bit_position in range(20):
        if ((ord(message[bit_position/8])) & 1<<(bit_position%8)) == 1<<(bit_position%8):
            char_array += '1'
        else:
            char_array += '0'
    return char_array

