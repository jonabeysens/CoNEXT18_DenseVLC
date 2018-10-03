%%  -- Code to reproduce figure 5 -- 
clear all;
close all;

%% Initialization
setup_placement = 'tx_placement_6x6.mat';
no_rx = 4;
initialize_components;

%% Load data
load('assign_tx_4_users_1pos');


%% FIGURE 5: Plot the illumination distribution and position of TXs
createFigure(400,300)
EvRoom = channel.visLuxRoom(tx,rx(1),room,0);
caxis([0 max(max(EvRoom))])
hold on;
for j=1:length(tx)
     scatter(tx(j).pos(1),tx(j).pos(2),20,'filled','MarkerEdgeColor',[1 1 1],...
    'MarkerFaceColor',[1 1 1],...
    'LineWidth',5);
    text(tx(j).pos(1)-0.15,tx(j).pos(2)+0.18,['TX',num2str(tx(j).id)], 'Color', 'black','FontWeight', 'Bold')  
end
hold off;
box on;


%% check the average illumination and uniformity level
spacing_boundary = 40; %unit is cm
EvRoom_area = EvRoom(spacing_boundary:(end-spacing_boundary),spacing_boundary:(end-spacing_boundary));
min_EvRoom_area = min(min(EvRoom_area));
% the average illumination
mean_EvRoom_area = mean2(EvRoom_area)
% the uniformity level
uniformity_EvRoom_area = min_EvRoom_area/mean_EvRoom_area