def generateField(min,max,stepsize,netheight):
    fieldX = []
    fieldY = []
    
    vpos = int((max+min)/2)
    for i in range(min,vpos,stepsize):
        # Make line
        xval = i & 0xfff
        yval = 0
        fieldX.append(xval)
        fieldY.append(yval)

    # make vertical line
    for i in range(0,netheight,stepsize):
        # Make line
        xval = vpos
        yval = i & 0xfff
        fieldX.append(xval)
        fieldY.append(yval)
        
    # Continue horizontal line
    for i in range(vpos,max,stepsize):
        # Make line
        xval = i & 0xfff
        yval = 0
        fieldX.append(xval)
        fieldY.append(yval)

    return fieldX,fieldY,vpos


def main():
    minval = 64
    maxval = 0xF00
    netheight = int(maxval/6)
    fx,fy,netpos = generateField(minval,maxval,2,netheight)
    print("const uint16_t fieldX[FIELDLENGTH] ={")
    print(",".join([str(v) for v in fx]))
    print("};")
    print("const uint16_t fieldY[FIELDLENGTH] ={")
    print(",".join([str(v) for v in fy]))
    print("};")

    print(f"#define NETPOS {netpos}")
    print(f"#define NETHEIGHT {netheight+minval}")
    print(f"#define MINDACVAL {minval}")
    print(f"#define MAXDACVAL {maxval}")

    print(f"#define FIELDLENGTH {len(fx)}")

if __name__ == "__main__":
    main()