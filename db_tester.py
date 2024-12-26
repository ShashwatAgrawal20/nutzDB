# A simple shitty script I used for testing the insertion limits and stuff
# This shit is not here to stay might replace this crap with some proper tests
# in the future, but hey, it's works as of now and I'm happy with it.


import pyautogui
import time
import random
import string

time.sleep(2)

for number in range(1, 111):
    length = random.randint(0, 55)
    random_word = ''.join(random.choice(string.ascii_lowercase)
                          for _ in range(length))
    query = f"insert {number} {random_word}{length}"
    pyautogui.write(query)
    pyautogui.press('enter')
