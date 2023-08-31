#!/bin/python
import sys
import socket
import cv2 as cv
import time


class VideoServer:
    def __init__(self, video_file):
        self._cap = cv.VideoCapture(video_file)
        self._width = int(self._cap.get(cv.CAP_PROP_FRAME_WIDTH))
        self._height = int(self._cap.get(cv.CAP_PROP_FRAME_HEIGHT))
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(("127.0.0.1", 3456))
        s.listen(10)
        self._socket = s

    def run(self):
        s, _ = self._socket.accept()
        s.send(self._width.to_bytes(4))
        s.send(self._height.to_bytes(4))

        request_next_frame = 0
        try:
            request_next_frame = int.from_bytes(s.recv(4))
        except:
            s.close()
            self._socket.close()
            self._cap.release()

        while True:
            ret, frame = self._cap.read()
            if not ret:
                break
            if request_next_frame != 0:
                if sys.byteorder == 'little':
                    frame_data = frame.byteswap().tobytes()
                else:
                    frame_data = frame.tobytes()
                s.send(frame_data)

            try:
                request_next_frame = int.from_bytes(s.recv(4))
            except:
                break

        s.shutdown(socket.SHUT_RDWR)
        s.close()
        self._socket.close()
        self._cap.release()


if __name__ == "__main__":
    server = VideoServer(sys.argv[1])
    server.run()
