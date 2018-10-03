%%  -- Code to reproduce figure 21 -- 
clear all;
close all;

no_rx = 4;
power_led = 74.42e-3; % as shown in Sec. 4.2

%% Load data
load('exp_raw_data_scenario_2','channel_data_all','s');
rng(s); % to make sure the randi function in getSINR returns the same result 

%% Compute swing levels based on the experimental channel data
for i=1:size(channel_data_all,1)
    for j=1:size(channel_data_all,3)
        [swings(i,j),var_high(i,j),var_low(i,j)] = getSwing(squeeze(channel_data_all(i,:,j)));
    end
end

%% SISO: Nearest TX communicating
for i=1:no_rx
    rx_id = i;
    [~,tx_id] = max(swings(:,i));
    alloc_matrix_exp_SISO(i,1) = tx_id;
    alloc_matrix_exp_SISO(i,2) = rx_id;
end

for u=1:no_rx
    SINR_SISO_single(1,u) = getSINR(alloc_matrix_exp_SISO,channel_data_all,swings,var_high,var_low,u);
end

%% D-MISO: All TX communicating
tx_rx(:,1) = [1 2 3 7 8 9 13 14 15]';
tx_rx(:,2) = [4 5 6 10 11 12 16 17 18]';
tx_rx(:,3) = [19 20 21 25 26 27 31 32 33]';
tx_rx(:,4) = [22 23 24 28 29 30 34 35 36]';

k=1;
for i=1:no_rx
    alloc_matrix_exp_ALL(k:k+length(tx_rx(:,i))-1,1) = tx_rx(:,i);
    alloc_matrix_exp_ALL(k:k+length(tx_rx(:,i))-1,2) = i;
    k=k+length(tx_rx(:,i));
end

for u=1:no_rx
    SINR_ALL_single(1,u) = getSINR(alloc_matrix_exp_ALL,channel_data_all,swings,var_high,var_low,u);
end

%% DenseVLC
kappa = 1.3;
alloc_matrix_exp = getAllocMatrix_SJR_exp(swings,kappa);
no_led_active = 1:36;

for u=1:no_rx
    for j=1:length(no_led_active)
        alloc_matrix_exp_tmp = alloc_matrix_exp(1:j,:);
        [SINR(j,u)] = getSINR(alloc_matrix_exp_tmp,channel_data_all,swings,var_high,var_low,u);
    end
end

%% FIGURE 21: Plot comparison in system throughput between three techniques

% DenseVLC
TP = log2(SINR);
TP(TP == -inf) = 0;
sys_TP = sum(TP,2);
sys_TP = [0 ; sys_TP]; 
sys_TP_norm = sys_TP/max(sys_TP);

% SISO
SINR_SISO = repmat(SINR_SISO_single,36+1,1);
TP_SISO = log2(SINR_SISO);
sys_TP_SISO = sum(TP_SISO,2);
sys_TP_SISO_norm = sys_TP_SISO/max(sys_TP);

% ALL
SINR_ALL = repmat(SINR_ALL_single,36+1,1);
TP_ALL= log2(SINR_ALL);
sys_TP_ALL = sum(TP_ALL,2);
sys_TP_ALL_norm = sys_TP_ALL/max(sys_TP);

figure;
plot([0 no_led_active].*power_led,sys_TP_norm);
hold on;
plot([0 no_led_active].*power_led,sys_TP_SISO_norm);
plot([0 no_led_active].*power_led,sys_TP_ALL_norm);
grid on;
xlabel('Total communication power of all transmitters (Pc) [W]');
ylabel('Normalized System Throughput');
legend('kappa-1.3','SISO','D-MISO','Location','Best');
axis([0 3 0 1.2])



