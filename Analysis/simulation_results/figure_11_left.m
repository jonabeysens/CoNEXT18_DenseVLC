%%  -- Code to reproduce Figure 11 (left) -- 
clear all;
% close all;

%% Initialization
setup_placement = 'tx_placement_6x6.mat';
no_rx = 4;
initialize_components;

%% Load data
load('assign_tx_4_users_1pos');

%% Perform ranking based on Signal to Jamming ratio (SJR)
kappa = [1.0 1.2 1.3 1.5];
for i=1:length(kappa)
    alloc_matrix(i,:,:) = sched.getAllocMatrix_SJR(tx,channel,rx,comm,kappa(i),0);
    [PTx_SJR,~,SINR_SJR,TP_system_SJR(i,:),logTP_system_SJR] = sched.calc_TP_alloc_matrix(tx,channel,rx,comm,squeeze(alloc_matrix(i,:,:)),B);
end


%% FIGURE 11 left: plot system throughput for different kappa values
figure;
plot(PTx_opt_cont,TP_system_opt_cont/1e6,'-', 'LineWidth',2);
grid on;
hold on;

for i=1:length(kappa)
    plot(PTx_SJR,TP_system_SJR(i,:)/1e6,'-', 'LineWidth',2);
end
hold off;
legend(['Optimal'; cellstr(strcat('Exponent-',num2str(kappa')))],'Location','Best');
xlabel('Total communication power of all transmitters (Pc) [W]');
ylabel('System Throughput [Mbit/s]');
