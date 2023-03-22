import subprocess
import shlex
processes = []
for i in range(1):
    #arguments += str(i) + "_image.jpg "
    processes.append(subprocess.Popen(shlex.split("./redisClient 10000 0.5 100 0 51000 0")))

for p in processes:
    p.wait()
#subprocess.call("./merge_resized_images")
