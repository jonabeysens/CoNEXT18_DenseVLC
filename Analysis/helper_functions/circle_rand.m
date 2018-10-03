function [x, y]=circle_rand(x1,y1,rc)
% selects a random point within the circle specified by center x1 and y1 and
% radius rc
%the function, must be on a folder in matlab path
a=2*pi*rand;
r=sqrt(rand);
x=(rc*r)*cos(a)+x1;
y=(rc*r)*sin(a)+y1;
end
