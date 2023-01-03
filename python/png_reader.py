import numpy as np
import matplotlib.pyplot as plt
import struct
import zlib


class Png():
    def __init__(self):
        self.width = 0
        self.height = 0
        self.chunks = []
        self.IDAT_data = []
        self.stride = 0
        self.bytesPerPixel = 0
        self.recon = []


class png_reader():
    def __init__(self, file):
        self.f = open(file, 'rb')
        self.png = Png()
        PngSignature = b'\x89PNG\r\n\x1a\n'
        if self.f.read(len(PngSignature)) != PngSignature:
            raise Exception('Invalid PNG Signature')

    def set_values(self, width, height, IDATA_data):
        self.png.width = width
        self.png.height = height
        self.png.IDAT_data = IDATA_data

    def read_chunk(self, f):
        chunk_length, chunk_type = struct.unpack('>I4s', f.read(8))
        chunk_data = f.read(chunk_length)
        checksum = zlib.crc32(chunk_data, zlib.crc32(
            struct.pack('>4s', chunk_type)))
        chunk_crc, = struct.unpack('>I', f.read(4))
        if chunk_crc != checksum:
            raise Exception('chunk checksum failed {} != {}'.format(chunk_crc,
                                                                    checksum))
        return chunk_type, chunk_data

    def build_chunk_list(self):
        while True:
            chunk_type, chunk_data = self.read_chunk(self.f)
            self.png.chunks.append((chunk_type, chunk_data))
            if chunk_type == b'IEND':
                break

    def processing_IHDR_chunk(self):
        _, IHDR_data = self.png.chunks[0]
        width, height, bitd, colort, compm, filterm, interlacem = struct.unpack(
            '>IIBBBBB', IHDR_data)
        if compm != 0:
            raise Exception('invalid compression method')
        if filterm != 0:
            raise Exception('invalid filter method')
        if colort != 6:
            raise Exception('we only support truecolor with alpha')
        if bitd != 8:
            raise Exception('we only support a bit depth of 8')
        if interlacem != 0:
            raise Exception('we only support no interlacing')

        IDAT_data = b''.join(chunk_data for chunk_type,
                             chunk_data in self.png.chunks if chunk_type == b'IDAT')
        IDAT_data = zlib.decompress(IDAT_data)
        self.set_values(width, height, IDAT_data)

    def paeth_predictor(self, a, b, c):
        p = a + b - c
        pa = abs(p - a)
        pb = abs(p - b)
        pc = abs(p - c)
        if pa <= pb and pa <= pc:
            Pr = a
        elif pb <= pc:
            Pr = b
        else:
            Pr = c
        return Pr

    def build_stride(self):
        self.build_chunk_list()
        self.processing_IHDR_chunk()
        self.png.bytesPerPixel = 4
        self.png.stride = self.png.width * self.png.bytesPerPixel

    def recon_a(self, r, c):
        return self.png.recon[r * self.png.stride + c - self.png.bytesPerPixel] if c >= self.png.bytesPerPixel else 0

    def recon_b(self, r, c):
        return self.png.recon[(r-1) * self.png.stride + c] if r > 0 else 0

    def recon_c(self, r, c):
        return self.png.recon[(r-1) * self.png.stride + c - self.png.bytesPerPixel] if r > 0 and c >= self.png.bytesPerPixel else 0

    def recon_pixel(self):
        self.build_stride()
        data = self.png.IDAT_data
        i = 0
        for r in range(self.png.height):
            filter_type = data[i]
            i += 1
            for c in range(self.png.stride):
                Filt_x = data[i]
                i += 1
                if filter_type == 0:
                    Recon_x = Filt_x
                elif filter_type == 1:
                    Recon_x = Filt_x + self.recon_a(r, c)
                elif filter_type == 2:
                    Recon_x = Filt_x + self.recon_b(r, c)
                elif filter_type == 3:
                    Recon_x = Filt_x + \
                        (self.recon_a(r, c) + self.recon_b(r, c)) // 2
                elif filter_type == 4:
                    Recon_x = Filt_x + \
                        self.paeth_predictor(
                            self.recon_a(r, c), self.recon_b(r, c), self.recon_c(r, c))
                else:
                    raise Exception('unknown filter type: ' + str(filter_type))
                self.png.recon.append(Recon_x & 0xff)

    def show_png(self):
        self.recon_pixel()
        plt.imshow(np.array(self.png.recon).reshape(
            self.png.height, self.png.width, 4))
        plt.show()


reader = png_reader('image.png')
reader.show_png()
