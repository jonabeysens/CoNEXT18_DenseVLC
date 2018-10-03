%%  -- Code to reproduce Figure 11 (right) -- 
clear all;
close all;

%% Initialization
setup_placement = 'tx_placement_6x6.mat';
no_rx = 4;
initialize_components;

%% Load data of the optimal solution
load('assign_tx_4_users_100pos.mat');

%% Perform calculations to find the heuristic solution
for zz = 1:no_pos
    % Set positions of RXs
    rx(1) = rx(1).setPosition(room,pos_rx_zz(1,:,zz));
    rx(2) = rx(2).setPosition(room,pos_rx_zz(2,:,zz));
    rx(3) = rx(3).setPosition(room,pos_rx_zz(3,:,zz));
    rx(4) = rx(4).setPosition(room,pos_rx_zz(4,:,zz));
    
    % Perform ranking based on Signal to Jamming ratio (SJR)
    exponent = [1 1.2 1.3 1.5];
    for i=1:length(exponent)
        alloc_matrix = sched.getAllocMatrix_SJR(tx,channel,rx,comm,exponent(i),0);
        [PTx_SJR,~,SINR_SJR,TP_system_SJR(i,:),logTP_system_SJR] = sched.calc_TP_alloc_matrix(tx,channel,rx,comm,alloc_matrix,B);
    end

    % Calculate loss in shannon capacity averaged over power levels
    for i=1:length(PTx_SJR)
        for j=1:length(exponent)
            [~,indx] = min(abs(PTx_SJR(i)-PTx_opt(zz,:)));
            loss_SJR(j,i) = (TP_system_SJR(j,i) - TP_system_opt(zz,indx))/TP_system_opt(zz,indx)*100;
        end
    end
    loss_SJR_mean(zz,:) = nanmean(loss_SJR,2)'; % compute mean over all the power levels
end

%% FIGURE 11 right: Plot histogram of loss in system throughput
figure;
for i=1:length(exponent)
    subplot(2,2,i)
    histfit(loss_SJR_mean(:,i),10,'kernel')
    yt = get(gca, 'YTick');
    set(gca, 'YTick', yt, 'YTickLabel', yt/length(loss_SJR_mean(:,i))*100);
    grid on;
    title(strcat('Exponent-',num2str(exponent(i))));
    xlabel('Throughput loss [%]');
    ylabel('Probability [%]');
end


