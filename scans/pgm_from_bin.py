width = 114
height = 57

with open("test.pgm", "wb") as image:
    image.write(bytes("P5\n{} {}\n255\n".format(width, height), 'ascii'))
    with open("raw_test.bin", "rb") as f:
        for i in range(width*height):
            image.write(bytes([(255-ord(f.read(1)))]))