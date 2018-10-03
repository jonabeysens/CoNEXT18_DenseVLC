close all;
clear all;

if(~isempty(instrfindall))
    fclose(instrfindall);
end

no_rx = 4;
buffer_out = 8*no_rx; % byte length % we send 8 bytes per RX
buffer_in = 36*no_rx*4; % data for no_rx RXs
mx = udp('192.168.7.127',1112,'LocalPort',1113,'OutputBufferSize', buffer_out, 'OutputDatagramPacketSize', buffer_out, ...
    'InputBufferSize', buffer_in, 'InputDatagramPacketSize', buffer_in); % mx
set(mx, 'Timeout',20);
if(~isempty(mx))
    fopen(mx);
end

while(1)
    command = input('Enter command:');
    if(command == 1) % perform channel measurement without sending back the information to the controller
        fwrite(mx,[cast(1,'uint8')],'uint8');
        disp('Channel measurement sent');

    elseif(command == 2) % set which LEDs should be active. This is required before an iperf measurment ist started.
        led_active = zeros(2,4);
        led_rx = cell(1,4);
        
        led_rx{1} = [1 2 7 8];
        led_rx{2} = [];
        led_rx{3} = [];
        led_rx{4} = [];
        
        for i=1:4
            for j=1:length(led_rx{i})
                led = led_rx{i}(j);
                rx = i;
                disp(strcat('Enable LED',num2str(led), ' to RX', num2str(rx)));
                if(led <=32)
                    led_active(1,rx) = bitor(led_active(1,rx),bitshift(1,led-1,'uint64'),'uint64');
                else
                    led_active(2,rx) = bitor(led_active(2,rx),bitshift(1,indx_sorted(2,rx)-32-1,'uint64'),'uint64');
                end
            end
        end
        data_to_tx = reshape(led_active,2*no_rx,1);
        fwrite(mx,[cast(3,'uint8')],'uint8');
        fwrite(mx,data_to_tx,'uint32');
        
    elseif(command == 3) % set which BBB should be the leading BBB (for the synchronization)
        fwrite(mx,[cast(33,'uint8') ; cast(1,'uint8') ; cast(2,'uint8') ; cast(4,'uint8'); cast(6,'uint8')],'uint8');
        
    elseif(command == 4) % Perform channel measurement from a single TX to all RXs. This is required to compute the swing levels and rank the TXs
        repeat = 0;
        polling_id = input('Polling ID:');
        while(1)
            disp(strcat('Polling ID: ',num2str(polling_id)));
            fwrite(mx,[cast(7,'uint8'); cast(polling_id,'uint8')],'uint8');
            channel_data_LED = zeros(48,4);
            swing_LED = zeros(1,4);
            var_high_LED = zeros(1,4);
            var_low_LED = zeros(1,4);
            for j=1:no_rx
                [channel_data_raw_bytes,count,msg] = fread(mx,[98],'uint8');
                if( channel_data_raw_bytes(1) == 0)
                    disp(strcat('no data received for RX'));
                else
                    rx_id = channel_data_raw_bytes(2) - 20;
                    disp(strcat('CHM measurement received for LED',num2str(channel_data_raw_bytes(1)),' from RX',num2str(channel_data_raw_bytes(2)-20)));
                    for i=1:48
                        channel_data_LED(i,rx_id) = channel_data_raw_bytes(2*i+1) * 2^8 + channel_data_raw_bytes(2*i+2);
                    end
                    [swing_LED(rx_id),var_high_LED(rx_id),var_low_LED(rx_id)] = getSwing(channel_data_LED(:,rx_id));
                    disp(strcat('swing=',num2str(swing_LED(rx_id))));
                end
            end
            saveRX = input('save?');
            if(saveRX == 1)
                channel_data_all(polling_id,:,:) = channel_data_LED;
                swing_all(polling_id,:) = swing_LED;
                var_high_all(polling_id,:) = var_high_LED;
                var_low_all(polling_id,:) = var_low_LED;
                repeat = 0;
                polling_id = polling_id + 1;
                if(polling_id > 36)
                    break;
                end
            elseif(saveRX == 99)
                break;
            else
                repeat = 1;
            end
        end
        
    elseif(command == 0)
        break;
    end
end

disp('Close sockets');
if(~isempty(mx))
    fclose(mx);
    delete(mx);
end