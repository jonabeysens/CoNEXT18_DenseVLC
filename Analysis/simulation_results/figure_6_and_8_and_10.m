%%  -- Code to reproduce Figure 6, 8 and 10 -- 
clear all;
close all;

%% Initialization
setup_placement = 'tx_placement_6x6.mat';
no_rx = 4;
initialize_components;
acquire = 0;

%% Acquire data
if(acquire)
    no_pos = 100;
    resolution = 30;
    %% Iterate over positions
    for zz = 1:no_pos
        disp([num2str(zz) '/' num2str(no_pos)]);
                
        tx8_pos = tx(8).getPosition();
        [pos_rx(1,1), pos_rx(1,2)] = circle_rand(tx8_pos(1),tx8_pos(2),0.25);
        rx(1) = rx(1).setPosition(room,pos_rx(1,:));

        tx10_pos = tx(10).getPosition();
        [pos_rx(2,1), pos_rx(2,2)] = circle_rand(tx10_pos(1),tx10_pos(2),0.25);
        rx(2) = rx(2).setPosition(room,pos_rx(2,:));

        tx20_pos = tx(20).getPosition();
        [pos_rx(3,1), pos_rx(3,2)] = circle_rand(tx20_pos(1),tx20_pos(2),0.25);
        rx(3) = rx(3).setPosition(room,pos_rx(3,:));

        tx22_pos = tx(22).getPosition();
        [pos_rx(4,1), pos_rx(4,2)] = circle_rand(tx22_pos(1),tx22_pos(2),0.25);
        rx(4) = rx(4).setPosition(room,pos_rx(4,:));
        
        pos_rx_zz(:,:,zz) = pos_rx;

        %% Perform general scheduling 
        % Put zeros for SRx and IRx because we don't build upon a previous step
        [PTx_opt_tmp,s_opt_tmp,SINR_opt_tmp,TP_system_opt_tmp,logTP_system_opt_tmp,~,~,PTx_max] = sched.max_MT_4rx(tx,channel,rx,comm,zeros(no_rx,1),zeros(no_rx,1),B,resolution,'logTP'); 
        
        %% Remove the discontinuities in the result (interpolate between valid results)
        PTx_opt_cont = PTx_opt_tmp;
        s_opt_cont = s_opt_tmp;
        SINR_opt_cont = SINR_opt_tmp;
        TP_system_opt_cont = TP_system_opt_tmp;
        logTP_system_opt_cont = logTP_system_opt_tmp;
        
        for i=2:length(TP_system_opt_cont)-1
            if(TP_system_opt_cont(i) < TP_system_opt_cont(i-1) && TP_system_opt_cont(i) < TP_system_opt_cont(i+1))
                TP_system_opt_cont(i) = mean([TP_system_opt_cont(i-1) TP_system_opt_cont(i+1)]);
                s_opt_cont(:,i) = mean([s_opt_cont(:,i-1) s_opt_cont(:,i+1)],2);
                logTP_system_opt_cont(i) = mean([logTP_system_opt_cont(i-1) logTP_system_opt_cont(i+1)]);
                SINR_opt_cont(1,i) = mean([SINR_opt_cont(1,i-1) SINR_opt_cont(1,i+1)]); % user 1
                SINR_opt_cont(2,i) = mean([SINR_opt_cont(2,i-1) SINR_opt_cont(2,i+1)]);
                SINR_opt_cont(3,i) = mean([SINR_opt_cont(3,i-1) SINR_opt_cont(3,i+1)]);
                SINR_opt_cont(4,i) = mean([SINR_opt_cont(4,i-1) SINR_opt_cont(4,i+1)]);
            end
        end
        
        PTx_opt(zz,:) = PTx_opt_cont;
        s_opt(zz,:,:) = s_opt_cont;
        SINR_opt(zz,:,:) = SINR_opt_cont;
        TP_system_opt(zz,:) = TP_system_opt_cont;
        logTP_system_opt(zz,:) = logTP_system_opt_cont;
    end
    save('assign_tx_4_users_100pos.mat');
else 
    %% Load data
    load('assign_tx_4_users_100pos.mat');
end

%% Calculate individual RX throughput
TP_1_opt = B*log2(squeeze(SINR_opt(:,1,:))'+1)';
TP_2_opt = B*log2(squeeze(SINR_opt(:,2,:))'+1)';
TP_3_opt = B*log2(squeeze(SINR_opt(:,3,:))'+1)';
TP_4_opt = B*log2(squeeze(SINR_opt(:,4,:))'+1)';

%% FIGURE 6: Plot the positions of TXs and RXs
createFigure(400,300)
hold on;
for j=1:length(tx)
     scatter(tx(j).pos(1),tx(j).pos(2),20,'filled','MarkerEdgeColor',[0 0 0],...
    'MarkerFaceColor',[0 0 0],...
    'LineWidth',5);
    text(tx(j).pos(1)-0.10,tx(j).pos(2)+0.17,['TX',num2str(tx(j).id)], 'Color', 'black','FontWeight', 'Bold')   
end
for i=1:no_pos
        plot(pos_rx_zz(1,1,i),pos_rx_zz(1,2,i),'square','Color',[1 0 0],...
                'MarkerSize',5, 'Linewidth', 1);
        plot(pos_rx_zz(2,1,i),pos_rx_zz(2,2,i),'o','Color',[0 0 1],...
                'MarkerSize',5, 'Linewidth', 1);
        plot(pos_rx_zz(3,1,i),pos_rx_zz(3,2,i),'+','Color',[0 1 0],...
                'MarkerSize',5, 'Linewidth', 1);
        plot(pos_rx_zz(4,1,i),pos_rx_zz(4,2,i),'x','Color',[1 0 1],...
        'MarkerSize',5, 'Linewidth', 1);
end
hold off;    
grid on; box on;
xlabel('position in x dimension [m]');
ylabel('position in y dimension [m]');


%% FIGURE 8: Plot the errorbar of system throughput (left) and individual throughput (right)
figure;
subplot(1,2,1)
TP_mean = mean(TP_system_opt/1e6,1);
for i=1:size(TP_system_opt,2)
    TP_std = std(TP_system_opt(:,i)/1e6);
    SEM = TP_std/sqrt(length(TP_system_opt(:,i)));               % Standard Error
    ts = tinv([0.025  0.975],length(TP_system_opt(:,i))-1);      % T-Score
    CI_down(i) = ts(1)*SEM;                                      % Confidence Intervals
    CI_up(i) = ts(2)*SEM;                                        
end

errorbar(PTx_max,TP_mean,abs(CI_down),CI_up);
grid on;
xlabel('Communication power [W]');
ylabel('Throughput [Mbit/s]');
legend('System','Location', 'Best');

[val,indx] = max(CI_up);
max_intv_system = 2*val/TP_mean(indx)*100;

% -- plot the errorbar of throughput RX1 --
subplot(1,2,2)
hold on;
TP_mean = mean(TP_1_opt/1e6,1);
for i=1:size(TP_1_opt,2)
    TP_std = std(TP_1_opt(:,i)/1e6);
    SEM = TP_std/sqrt(length(TP_1_opt(:,i)));               % Standard Error
    ts = tinv([0.025  0.975],length(TP_1_opt(:,i))-1);      % T-Score
    CI_down(i) = ts(1)*SEM;                                 % Confidence Intervals
    CI_up(i) = ts(2)*SEM;   
end

errorbar(PTx_max,TP_mean,abs(CI_down),CI_up);
grid on;
[val,indx] = max(CI_up);
max_intv_rx1 = 2*val/TP_mean(indx)*100;

% -- plot the errorbar of throughput RX2 --
TP_mean = mean(TP_2_opt/1e6,1);
for i=1:size(TP_2_opt,2)
    TP_std = std(TP_2_opt(:,i)/1e6);
    SEM = TP_std/sqrt(length(TP_2_opt(:,i)));               % Standard Error
    ts = tinv([0.025  0.975],length(TP_2_opt(:,i))-1);      % T-Score
    CI_down(i) = ts(1)*SEM;                                 % Confidence Intervals
    CI_up(i) = ts(2)*SEM;   
end

errorbar(PTx_max,TP_mean,abs(CI_down),CI_up);
grid on;
[val,indx] = max(CI_up);
max_intv_rx2 = 2*val/TP_mean(indx)*100;

% -- plot the errorbar of throughput RX3 --
TP_mean = mean(TP_3_opt/1e6,1);
for i=1:size(TP_3_opt,2)
    TP_std = std(TP_3_opt(:,i)/1e6);
    SEM = TP_std/sqrt(length(TP_3_opt(:,i)));               % Standard Error
    ts = tinv([0.025  0.975],length(TP_3_opt(:,i))-1);      % T-Score
    CI_down(i) = ts(1)*SEM;                                 % Confidence Intervals
    CI_up(i) = ts(2)*SEM;   
end

errorbar(PTx_max,TP_mean,abs(CI_down),CI_up);
grid on;
[val,indx] = max(CI_up);
max_intv_rx3 = 2*val/TP_mean(indx)*100;

% -- plot the errorbar of throughput RX4 --
TP_mean = mean(TP_4_opt/1e6,1);
for i=1:size(TP_4_opt,2)
    TP_std = std(TP_4_opt(:,i)/1e6);
    SEM = TP_std/sqrt(length(TP_4_opt(:,i)));               % Standard Error
    ts = tinv([0.025  0.975],length(TP_4_opt(:,i))-1);      % T-Score
    CI_down(i) = ts(1)*SEM;                                 % Confidence Intervals
    CI_up(i) = ts(2)*SEM;   
end

errorbar(PTx_max,TP_mean,abs(CI_down),CI_up);
grid on;
[val,indx] = max(CI_up);
max_intv_rx4 = 2*val/TP_mean(indx)*100;

xlabel('Communication power [W]');
ylabel('Throughput [Mbit/s]');
legend('RX1','RX2','RX3','RX4','Location', 'Best');


%% FIGURE 10: Plot the cdf of the data
rx_id = 2;
tx_id = [3 5 10 15];
s_sel = s_opt([86,50,78,94,61],:,:);
figure;
for j=1:length(tx_id)
    subplot(2,2,j)
    for i=1:size(s_sel,1)
        [prob,power] = ecdf(squeeze(s_sel(i,(rx_id-1)*length(tx)+tx_id(j),:)));
        plot(power,prob, 'LineWidth',1);
        hold on;
    end
    grid on;
    hold off;
    axis([0 1.2 0 1.1]);
    xlabel('Swing level [A]');
    ylabel('Empircal CDF');
    title(strcat('TX ',num2str(tx_id(j))));
    legend(cellstr(num2str([1:size(s_sel,1)]')),'Location', 'Best');
end

