%%  -- Code to reproduce figure 19 -- 
clear all;
close all;

no_rx = 4;
power_led = 74.42e-3; % as shown in Sec. 4.2
s=rng; % to make sure the randi function in getSINR returns the same result

%% Load data
load('exp_raw_data_scenario_2','channel_data_all');

%% Compute swing levels based on the experimental channel data
for i=1:size(channel_data_all,1)
    for j=1:size(channel_data_all,3)
        [swings(i,j),var_high(i,j),var_low(i,j)] = getSwing(squeeze(channel_data_all(i,:,j)));
    end
end


%% FIGURE 19 left: Plot normalized throughput for all RXs
kappa = 1.3;
alloc_matrix_exp = getAllocMatrix_SJR_exp(swings,kappa);
no_led_active = 1:36;

for u=1:no_rx
    for j=1:length(no_led_active)
        alloc_matrix_exp_tmp = alloc_matrix_exp(1:j,:);
        [SINR(j,u)] = getSINR(alloc_matrix_exp_tmp,channel_data_all,swings,var_high,var_low,u);
    end
end
TP = log2(SINR);
TP(TP == -inf) = 0;
TP = [zeros(1,4) ; TP]; 
TP = TP./max(TP,[],1);

figure;
plot([0 no_led_active].*power_led,TP);
legend([cellstr(strcat('RX',num2str([1:4]')))],'Location','Best');
xlabel('Total communication power of all transmitters (Pc) [W]');
ylabel('Normalized Throughput');
grid on;


%% FIGURE 19 right: Plot normalized system throughput for several kappa values
kappa_all = [1.0 1.2 1.3 1.5];
for e=1:length(kappa_all)
    alloc_matrix_exp = getAllocMatrix_SJR_exp(swings,kappa_all(e));
    no_led_active = 1:36;

    for u=1:4
        for j=1:length(no_led_active)
            alloc_matrix_exp_tmp = alloc_matrix_exp(1:j,:);
            [SINR(j,u)] = getSINR(alloc_matrix_exp_tmp,channel_data_all,swings,var_high,var_low,u);
        end
    end
    TP = log2(SINR);
    TP(TP == -inf) = 0;
    sys_TP_tmp = sum(TP,2);
    sys_TP_tmp= [0 ; sys_TP_tmp]; 
    sys_TP(:,e) = sys_TP_tmp/max(sys_TP_tmp);
end

figure;
plot([0 no_led_active].*power_led,sys_TP);
grid on;
xlabel('Total communication power of all transmitters (Pc) [W]');
ylabel('Normalized System Throughput');
legend([cellstr(strcat('kappa-',num2str(kappa_all')))],'Location','Best');

