%%  -- Code to generate mat-file tx_placement_6x6.mat -- 
clear all;
close all;

%% Initialization
% Tx
no_tx_horz = 6; 
no_tx_vert = 6; 
part_no = 6;
no_leds_per_TX = 1;
height_tx = 2.8;
tx_semiAngle = 15;
I_bias = 450e-3;

% Rx
height_rx = 0.8;

% Room
room_dim = [10 10 height_tx];

% Constructors
room = room(1,room_dim,no_tx_horz,no_tx_vert);
tx = transmitter(1,no_leds_per_TX,[room_dim(1)/2 room_dim(2)/2 height_tx],[-90,-90],I_bias,tx_semiAngle);
rx = receiver(1,[90,0],height_rx,room);
channel = channel();


% boundaries of the area of interest
distBoundary = 0.25;
distGrid_x = 0.25;
distGrid_y = 0.25;

% Propose area-of-interest
room_dim = [2*distBoundary+(no_tx_horz-1)*2*distGrid_x ...
            2*distBoundary+(no_tx_vert-1)*2*distGrid_y ...
            height_tx];

% TX positions    
pos_tx = zeros(no_tx_vert*no_tx_horz,3);
for j=1:no_tx_vert
    for i=1:no_tx_horz
        pos_tx(i+(j-1)*no_tx_horz,:) = [distBoundary+(i-1)*2*distGrid_x ...
                     distBoundary+(j-1)*2*distGrid_y ...
                     height_tx];
    end
end

%% Verify the proposed area-of-interest
room = room.setDimensions(room_dim);
clear tx;
for i=1:no_tx_horz*no_tx_vert
    tx(i) = transmitter(i,no_leds_per_TX,[room_dim(1)/2 room_dim(2)/2 height_tx],[-90,-90],I_bias,tx_semiAngle);
end

for i=1:length(tx)
    tx(i) = tx(i).setPosition(pos_tx(i,:));
end

EvRoom = channel.visLuxRoom(tx,rx,room,1);
for i=1:length(tx)
    tx(i).visPosTx(1,0);
end


%% check mean and uniformity in area of interest
spacing_boundary = 40; %unit is cm
EvRoom_area = EvRoom(spacing_boundary:(end-spacing_boundary),spacing_boundary:(end-spacing_boundary));
min_EvRoom_area = min(min(EvRoom_area));
mean_EvRoom_area = mean2(EvRoom_area)
uniformity_EvRoom_area = min_EvRoom_area/mean_EvRoom_area

%% optionally save configuration
% save(strcat('tx_placementt_',num2str(no_tx_horz),'x',num2str(no_tx_vert),'.mat'),'no_tx_horz','no_tx_vert','no_leds_per_TX','pos_tx','distGrid_x','distGrid_y',...
%     'distBoundary','tx_semiAngle','room_dim','mean_EvRoom_area','min_EvRoom_area','uniformity_EvRoom_area','I_bias','part_no',...
%     'spacing_boundary');




