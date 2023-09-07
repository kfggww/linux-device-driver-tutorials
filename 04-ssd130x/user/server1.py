#!/bin/python
import sys
import socket
import cv2 as cv
import subprocess
import threading
import time


class VideoServer:
    def __init__(self, video_file):
        self._cap = cv.VideoCapture(video_file)
        self._width = int(self._cap.get(cv.CAP_PROP_FRAME_WIDTH))
        self._height = int(self._cap.get(cv.CAP_PROP_FRAME_HEIGHT))
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(("127.0.0.1", 3456))
        s.listen(10)
        self._listen_socket = s

    def run(self):
        s, _ = self._listen_socket.accept()
        s.send(self._width.to_bytes(4))
        s.send(self._height.to_bytes(4))

        self._listen_socket.shutdown(socket.SHUT_RDWR)
        self._listen_socket.close()

        request_next_frame = 0
        try:
            request_next_frame = int.from_bytes(s.recv(4))
        except:
            s.close()
            self._cap.release()

        while True:
            ret, frame = self._cap.read()
            if not ret:
                break
            if request_next_frame != 0:
                frame_data = frame.tobytes()
                s.send(frame_data)

            try:
                request_next_frame = int.from_bytes(s.recv(4))
            except:
                break

        s.shutdown(socket.SHUT_RDWR)
        s.close()
        self._cap.release()


def run_video_server(video_file):
    server = VideoServer(video_file)
    server.run()


def run_server2(threshold):
    args = ["./server2"]
    if threshold is not None:
        args.append(threshold)

    subprocess.call(args)


if __name__ == "__main__":
    t1 = threading.Thread(target=run_video_server, args=(sys.argv[1], ))
    t2 = threading.Thread(
        target=run_server2, args=(sys.argv[2] if len(sys.argv) >= 3 else None,))

    t1.start()
    time.sleep(1.0)
    t2.start()

    t2.join()
    t1.join()
