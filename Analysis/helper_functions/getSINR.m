function [SINR] = getSINR(alloc_matrix_exp,channel_data_all,swings,var_high,var_low,desired_rx)
% Get the signal-to-interference-and-noise ratio based on the raw channel
% data
    l=1;k=1;
    swing_desired = 0;
    power_noise = 0;
    first = 1;
    sum_raw_interfering = 0;
    for i=1:size(alloc_matrix_exp,1)
        tx_id  = alloc_matrix_exp(i,1);
        rx_id  = alloc_matrix_exp(i,2);

        if(rx_id == desired_rx)
            swing_desired(k,1) = swings(tx_id,desired_rx);
            if first == 1
                power_noise = max([var_high(tx_id,desired_rx),var_low(tx_id,desired_rx)]);
                first = 0;
            end
            k=k+1;
        else
            sum_raw_interfering = sum_raw_interfering + ...
                circshift(squeeze(channel_data_all(tx_id,:,desired_rx)),randi(48));
            l=l+1;
        end
    end
    power_interference = var(sum_raw_interfering);
    SINR = sum(swing_desired)^2/(power_interference+(power_noise+50)); % 50 is used as an offset
end
