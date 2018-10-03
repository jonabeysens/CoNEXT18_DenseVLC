%%  -- Code to reproduce Figure 7 and 9 -- 
clear all;
close all;

%% Initialization
setup_placement = 'tx_placement_6x6.mat';
no_rx = 4;
initialize_components;
acquire = 0;


%% Acquire data
if(acquire)
    resolution = 50;
    pos_fixed = 1;
    centered_around_tx = [8 10 20 22];
    radius = 0;
    %% Set position of RXs
    if(pos_fixed)
        pos_rx = [0.916824562267904 0.921042557373336;1.65 0.65;0.717072331287472 1.931922685730696;1.986870932381739 1.685210380132613];
        for i=1:length(rx)
            rx(i) = rx(i).setPosition(room,pos_rx(i,:));
        end
    else
        tx1_pos = tx(centered_around_tx(1)).getPosition();
        [pos_rx(1,1), pos_rx(1,2)] = circle_rand(tx1_pos(1),tx1_pos(2),radius);
        rx(1) = rx(1).setPosition(room,pos_rx(1,:));

        tx2_pos = tx(centered_around_tx(2)).getPosition();
        [pos_rx(2,1), pos_rx(2,2)] = circle_rand(tx2_pos(1),tx2_pos(2),radius);
        rx(2) = rx(2).setPosition(room,pos_rx(2,:));

        tx3_pos = tx(centered_around_tx(3)).getPosition();
        [pos_rx(3,1), pos_rx(3,2)] = circle_rand(tx3_pos(1),tx3_pos(2),radius);
        rx(3) = rx(3).setPosition(room,pos_rx(3,:));

        tx4_pos = tx(centered_around_tx(4)).getPosition();
        [pos_rx(4,1), pos_rx(4,2)] = circle_rand(tx4_pos(1),tx4_pos(2),radius);
        rx(4) = rx(4).setPosition(room,pos_rx(4,:));
    end

    %% Perform general scheduling 
    tic
    % Put zeros for SRx and IRx because we don't build upon a previous step
    [PTx_opt,s_opt,SINR_opt,TP_system_opt,logTP_system_opt,exitflag,fval,PTx_max] = sched.max_MT_4rx(tx,channel,rx,comm,zeros(no_rx,1),zeros(no_rx,1),B,resolution,'logTP'); 
    toc
    
        %% remove the discontinuities in the result (interpolate between valid results)
    PTx_opt_cont = PTx_opt;
    SINR_opt_cont = SINR_opt;
    TP_system_opt_cont = TP_system_opt;
    logTP_system_opt_cont = logTP_system_opt;
    s_opt_cont = s_opt;
    for i=2:length(TP_system_opt_cont)-1
        if(TP_system_opt_cont(i) < TP_system_opt_cont(i-1) && TP_system_opt_cont(i) < TP_system_opt_cont(i+1))
            TP_system_opt_cont(i) = mean([TP_system_opt_cont(i-1) TP_system_opt_cont(i+1)]);
            logTP_system_opt_cont(i) = mean([logTP_system_opt_cont(i-1) logTP_system_opt_cont(i+1)]);
            s_opt_cont(:,i) = mean([s_opt_cont(:,i-1) s_opt_cont(:,i+1)],2);
            SINR_opt_cont(1,i) = mean([SINR_opt_cont(1,i-1) SINR_opt_cont(1,i+1)]); % user 1
            SINR_opt_cont(2,i) = mean([SINR_opt_cont(2,i-1) SINR_opt_cont(2,i+1)]);
            SINR_opt_cont(3,i) = mean([SINR_opt_cont(3,i-1) SINR_opt_cont(3,i+1)]);
            SINR_opt_cont(4,i) = mean([SINR_opt_cont(4,i-1) SINR_opt_cont(4,i+1)]);
        end
    end
    save('assign_tx_4_users_1pos');   
else
    %% Load data
    load('assign_tx_4_users_1pos');
end

%% FIGURE 7: Plot the position of TXs and RXs
createFigure(400,300)
hold on;
for j=1:length(tx)
     scatter(tx(j).pos(1),tx(j).pos(2),30,'filled','MarkerEdgeColor',[0 0 0],...
    'MarkerFaceColor',[0 0 0],...
    'LineWidth',3);
    text(tx(j).pos(1)-0.10,tx(j).pos(2)+0.17,['TX',num2str(tx(j).id)], 'Color', 'black')%,'FontWeight', 'Bold')   
end
plot(rx(1).pos(1),rx(1).pos(2),'square','Color',[1 0 0],...
        'MarkerSize',8, 'Linewidth', 1);
text(rx(1).pos(1)-0.10,rx(1).pos(2)+0.15,strcat('RX ',num2str(rx(1).getID())), 'Color', [1 0 0], 'FontWeight', 'Bold')
plot(rx(2).pos(1),rx(2).pos(2),'o','Color',[0 0 1],...
        'MarkerSize',8, 'Linewidth', 1);
text(rx(2).pos(1)-0.10,rx(2).pos(2)+0.15,strcat('RX ',num2str(rx(2).getID())), 'Color', [0 0 1], 'FontWeight', 'Bold')
plot(rx(3).pos(1),rx(3).pos(2),'*','Color',[0 1 0],...
        'MarkerSize',8, 'Linewidth', 1);
text(rx(3).pos(1)-0.10,rx(3).pos(2)+0.15,strcat('RX ',num2str(rx(3).getID())), 'Color', [0 1 0], 'FontWeight', 'Bold')
plot(rx(4).pos(1),rx(4).pos(2),'x','Color',[1 0 1],...
'MarkerSize',8, 'Linewidth', 1);
text(rx(4).pos(1)-0.10,rx(4).pos(2)+0.15,strcat('RX ',num2str(rx(4).getID())), 'Color', [1 0 1], 'FontWeight', 'Bold')

grid on; box on;
xlabel('position in x dimension [m]');
ylabel('position in y dimension [m]');


%% FIGURE 9: Zoom in to swing levels of 2 RXs
createFigure(500,300)
max_tx_id = 18;
s1 = subplot(1,2,1);
imagesc(PTx_opt_cont,1:max_tx_id,s_opt_cont(1:max_tx_id,:));
set(gca, 'Ydir', 'normal');
set(gca,'ytick',1:max_tx_id);
colormap gray;
c = colorbar;
c.Label.String = 'Swing current level [A]';
xlabel('Communication power [W]');
ylabel('Transmitter index');

s2 = subplot(1,2,2);
imagesc(PTx_opt_cont,1:max_tx_id,s_opt_cont(length(tx)+1:length(tx)+max_tx_id,:));
set(gca, 'Ydir', 'normal');
set(gca,'ytick',1:max_tx_id);
colormap gray;
c = colorbar;
c.Label.String = 'Swing current level [A]';
xlabel('Communication power [W]');
ylabel('Transmitter index');
