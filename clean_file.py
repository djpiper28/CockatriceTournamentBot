import sys

def main(argv):
    for arg in argv:
        print(f"Cleaning up '{arg}'.")
        
        f = open(arg, "r")
        lines = []
        
        for line in f:
            lines.append(f'#{line}'.strip()[1:])
            
        f.close()
        f = open(arg, "w")
        
        f.write("\n".join(lines))
        f.close()

if __name__ == "__main__":
   main(sys.argv[1:])
