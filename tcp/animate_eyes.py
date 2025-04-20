class Frame:
    def __init__(self,x_size,y_size):
        self.x_size = x_size
        self.y_size = y_size
        self.left_data = [[0 for _ in range(self.x_size)] for _ in range(self.y_size)]
        self.right_data = [[0 for _ in range(self.x_size)] for _ in range(self.y_size)]
    def __str__(self):
        return "\n".join(["".join(["⚪" if i==1 else "⚫" for i in  self.left_data[j]] + ["\t","\t"] + ["⚪" if i==1 else "⚫" for i in self.right_data[j]]) for j in range(len(self.left_data))])
    def reset(self):
        self.left_data = [[0 for _ in range(self.x_size)] for _ in range(self.y_size)]
        self.right_data = [[0 for _ in range(self.x_size)] for _ in range(self.y_size)]
    def set_point(self,x,y,val,option):
        if x < self.x_size and y< self.y_size:
            if option == "left":
                self.left_data[y][x] = val
            elif option == "right":
                self.right_data[y][x] = val
            elif option == "both":
                self.left_data[y][x] = val
                self.right_data[y][x] = val
    def set_area(self,x_min,x_max,y_min,y_max,val,option):
        if 0 <= x_min < self.x_size and 0 <= x_max < self.x_size and 0 <= y_min < self.y_size and 0 <= y_max < self.y_size:
            if option == "left":
                [[self.set_point(j,i,val,"left") for j in range(x_min,x_max+1)] for i in range(y_min,y_max+1)]
            elif option == "right":
                [[self.set_point(j,i,val,"right") for j in range(x_min,x_max+1)] for i in range(y_min,y_max+1)]
            elif option == "both":
                [[self.set_point(j,i,val,"both") for j in range(x_min,x_max+1)] for i in range(y_min,y_max+1)]
    def get_output(self):
        output = sum([self.left_data[i] if i%2==0 else self.left_data[i][::-1] for i in range(len(self.left_data))] + \
        [self.right_data[i] if i%2==0 else self.right_data[i][::-1] for i in range(len(self.right_data))],[])
        return ",".join(str(item) for item in output)

case_1 = Frame(7,7)

# Normal Eyes (Can Blink, Can Look)
case_1.set_area(2,4,1,5,1,"both")
print(case_1)
print(case_1.get_output())
case_1.reset()