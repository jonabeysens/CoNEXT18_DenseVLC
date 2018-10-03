classdef scheduling
    methods
        %% Constructor
        function obj = scheduling()
        end
        
        %% Choose swing LEDs to achieve max metric (MT) for 4 users
        % input
        % output
        %   PTx_opt:        1 x resolution
        %   s_opt:          2*no_rx x resolution
        %   SINR_opt:       no_rx x resolution
        %   TP_system:      1 x resolution
        %   logTP_system:   1 x resolution
        function [PTx_opt,s_opt,SINR_opt,TP_system,logTP_system,exitflag,fval,PTx_max] = max_MT_4rx(obj,tx,channel,rx,comm,SRx,IRx,B,resolution,metric)
            no_tx = length(tx);
            if(no_tx == 0)
                PTx_opt = [];
                s_opt  = [];
                SINR_opt = [];
                TP_system = [];
                logTP_system = [];   
                return
            end
            
            % put the channels to the receiver under each other
            h(1:no_tx,1) = channel.getPathLoss(tx,rx(1))';              % channels to RX 1
            h(no_tx+1:2*no_tx,1) = channel.getPathLoss(tx,rx(2))';      % channels to RX 2
            h(2*no_tx+1:3*no_tx,1) = channel.getPathLoss(tx,rx(3))';    % channels to RX 3
            h(3*no_tx+1:4*no_tx,1) = channel.getPathLoss(tx,rx(4))';    % channels to RX 4
            
            max_swing = tx(1).max_swing;
            r = tx(1).dyn_resistance;
            P = linspace(0.01,no_tx*tx(1).no_leds*r/4*max_swing^2,resolution); % cannot start from zero
            wait = waitbar(0,'Initializing waitbar...');
            
            % initalize variables            
            SRx_1 = SRx(1);
            IRx_1 = IRx(1);
            SRx_2 = SRx(2);
            IRx_2 = IRx(2);
            SRx_3 = SRx(3);
            IRx_3 = IRx(3);
            SRx_4 = SRx(4);
            IRx_4 = IRx(4);
            no_leds = tx(1).no_leds;
            R = rx(1).R;
            WPE = tx(1).WPE;
            noise_power = comm.getRxNoisePower();
            
            % set up helper functions for optimization 
            sinr_1 = @(s) (SRx_1 + R*WPE*no_leds*r/4*s(1:no_tx).^2'*h(1:no_tx))^2/ ...
                                (noise_power + (IRx_1 ...
                                + R*WPE*no_leds*r/4*s(no_tx+1:2*no_tx).^2'*h(1:no_tx) ...               % Inteference from transmitters allocated to user 2
                                + R*WPE*no_leds*r/4*s(2*no_tx+1:3*no_tx).^2'*h(1:no_tx) ...             % Inteference from transmitters allocated to user 3
                                + R*WPE*no_leds*r/4*s(3*no_tx+1:4*no_tx).^2'*h(1:no_tx))^2);            % Inteference from transmitters allocated to user 4
            sinr_2 = @(s) (SRx_2 + R*WPE*no_leds*r/4*s(no_tx+1:2*no_tx).^2'*h(no_tx+1:2*no_tx))^2/ ...
                                (noise_power + (IRx_2 + ...
                                + R*WPE*no_leds*r/4*s(1:no_tx).^2'*h(no_tx+1:2*no_tx) ...               % Inteference from transmitters allocated to user 1
                                + R*WPE*no_leds*r/4*s(2*no_tx+1:3*no_tx).^2'*h(no_tx+1:2*no_tx) ...     % Inteference from transmitters allocated to user 3
                                + R*WPE*no_leds*r/4*s(3*no_tx+1:4*no_tx).^2'*h(no_tx+1:2*no_tx))^2);    % Inteference from transmitters allocated to user 4
                            
            sinr_3 = @(s) (SRx_3 + R*WPE*no_leds*r/4*s(2*no_tx+1:3*no_tx).^2'*h(2*no_tx+1:3*no_tx))^2/ ...
                                (noise_power + (IRx_3 + ...
                                + R*WPE*no_leds*r/4*s(1:no_tx).^2'*h(2*no_tx+1:3*no_tx) ...               % Inteference from transmitters allocated to user 1
                                + R*WPE*no_leds*r/4*s(no_tx+1:2*no_tx).^2'*h(2*no_tx+1:3*no_tx) ...       % Inteference from transmitters allocated to user 2
                                + R*WPE*no_leds*r/4*s(3*no_tx+1:4*no_tx).^2'*h(2*no_tx+1:3*no_tx))^2);    % Inteference from transmitters allocated to user 4
                            
            sinr_4 = @(s) (SRx_4 + R*WPE*no_leds*r/4*s(3*no_tx+1:4*no_tx).^2'*h(3*no_tx+1:4*no_tx))^2/ ...
                                (noise_power + (IRx_4 + ...
                                + R*WPE*no_leds*r/4*s(1:no_tx).^2'*h(3*no_tx+1:4*no_tx) ...               % Inteference from transmitters allocated to user 1
                                + R*WPE*no_leds*r/4*s(no_tx+1:2*no_tx).^2'*h(3*no_tx+1:4*no_tx) ...       % Inteference from transmitters allocated to user 2
                                + R*WPE*no_leds*r/4*s(2*no_tx+1:3*no_tx).^2'*h(3*no_tx+1:4*no_tx))^2);    % Inteference from transmitters allocated to user 3
            
            if(strcmp(metric,'TP'))
                fun = @(s) -(log2(sinr_1(s) + 1) + log2(sinr_2(s) + 1) + log2(sinr_3(s) + 1) + log2(sinr_4(s) + 1)); % no bandwidth here because is a constant
            elseif(strcmp(metric,'logTP'))
                fun = @(s) -(log10(log2(sinr_1(s) + 1)) + log10(log2(sinr_2(s) + 1)) + log10(log2(sinr_3(s) + 1)) + log10(log2(sinr_4(s) + 1))); % no bandwidth here because is a constant
            else
                error('Specified metric is unknown');
            end        
            power_total = @(s) no_leds*r/4*sum(s.^2);
            % perform the optimization
            s0 = zeros(4*no_tx,1);
            for i=1:length(P)
                waitbar(i/length(P),wait,sprintf('%d%% along...',uint8(i/length(P)*100)))

                A = [eye(no_tx) eye(no_tx) eye(no_tx) eye(no_tx)];
                b = ones(no_tx,1)*max_swing;
                lb = zeros(4*no_tx,1);
                ub = [];
                nonlcon=@(s) deal(power_total(s)-P(i),0);
                options = optimoptions('fmincon','MaxFunctionEvaluations',40000, 'Display', 'notify-detailed','StepTolerance',1e-12,'UseParallel',true,'Algorithm','interior-point','HessianApproximation','bfgs'); % 3000
                [s_opt(:,i),fval(i),exitflag(i),~] = fmincon(fun,s0,A,b,[],[],lb,ub,nonlcon,options);
                
                PTx_opt(i) = getTotalPower(obj,s_opt(:,i),no_tx,4,r,no_leds);%tx(1).no_leds*r/4*sum((s_opt(1:no_tx,i)+s_opt(no_tx+1:2*no_tx,i)).^2);
                SINR_opt(:,i) = [sinr_1(s_opt(:,i));sinr_2(s_opt(:,i));sinr_3(s_opt(:,i));sinr_4(s_opt(:,i))];%0;%(rx(1).R*tx(1).WPE*tx(1).no_leds*r*x_opt(:,i)'*h(:,1))^2/comm.getRxNoisePower(); 
            end              
            TP_system = B*log2(SINR_opt(1,:)+1)+B*log2(SINR_opt(2,:)+1)+B*log2(SINR_opt(3,:)+1)+B*log2(SINR_opt(4,:)+1);
            logTP_system = log10(B*log2(SINR_opt(1,:)+1))+log10(B*log2(SINR_opt(2,:)+1))+log10(B*log2(SINR_opt(3,:)+1))+log10(B*log2(SINR_opt(4,:)+1));
            PTx_max = P;
            close(wait);
        end
        
        
        function power = getTotalPower(obj,s,no_tx,no_rx,r,no_leds)
            s_ma = reshape(s,[no_tx,no_rx]);
            power =  no_leds*r/4*sum(sum(s_ma,2).^2);
        end
        
        
        %% Get the allocation matrix for the ranking approach based on Signal to Jamming ratio
        function alloc_matrix = getAllocMatrix_SJR(obj,tx,channel,rx,comm,exponent,advanced)
            no_tx = length(tx);
            no_rx = length(rx);

            for i=1:length(rx)
                h(:,i) = channel.getPathLoss(tx,rx(i))';    
            end
            
            for i=1:length(tx)
                for j=1:length(rx)
                    h_jam = h(i,:);
                    h_jam(j) = [];
                    if(advanced)
                        exponent = h(i,j)/sum(h_jam);
                        SJR(i,j) = h(i,j)^exponent/sum(h_jam);
                    else
                        SJR(i,j) = h(i,j)^exponent/(h(i,j) + sum(h_jam));
                    end
                    
                end
            end
            
            SJR_array = reshape(SJR,[],1);
            % Construct the allocation matrix
            for i=1:length(SJR_array)/no_rx
                [val,indx] = max(SJR_array);
                tx_id_tmp = mod(indx,no_tx);
                if(tx_id_tmp ~= 0)
                    tx_id = tx_id_tmp;
                else
                    tx_id = no_tx;
                end
                rx_id = ceil(indx/no_tx);
                alloc_matrix(i,:) = [tx_id rx_id val];
                SJR_array(tx_id:no_tx:end) = -inf;
            end
        end
        
        %% Compute the SINR ratio based on the allocation matrix
        % input
        % output
        %   PTx:            1 x (no_alloc_tx + 1) | +1 because of zero power
        %   SINR:           no_rx x (no_alloc_tx + 1) | +1 because of zero power
        %   TP_system:      1 x (no_alloc_tx + 1) | +1 because of zero power
        %   logTP_system:   1 x (no_alloc_tx + 1) | +1 because of zero power
        function [PTx,s,SINR,TP_system,logTP_system] = calc_TP_alloc_matrix(obj,tx,channel,rx,comm,alloc_matrix,B) 
            % computation of SINR based on allocation matrix and the
            % channel h. Tx can swing with zero or max swing. 
            % Allocation matrix has on every row the TX id in column 1 and
            % RX id in column 2
            
            no_tx = length(tx); % number of all TX
            no_rx = length(rx); % number of all RX
            
            no_alloc_tx = size(alloc_matrix,1); % number of TX to be allocated
            
            max_swing = tx(1).max_swing;
            r = tx(1).dyn_resistance; 
            power_per_tx = tx(1).no_leds*r/4*max_swing^2;
            s = zeros(no_tx*no_rx,no_alloc_tx); % vector of all swing levels, similar as in optimization problem
            power_tx = zeros(no_tx,no_rx);
            
            for i=1:length(rx)
                h(:,i) = channel.getPathLoss(tx,rx(i))';    
            end
            
            for i=1:size(alloc_matrix,1)
                s(alloc_matrix(i,1) + (alloc_matrix(i,2)-1)*no_tx,i:end) = max_swing;
                power_tx(alloc_matrix(i,1),alloc_matrix(i,2)) = power_per_tx;
                for j=1:no_rx
                    sig_desired = rx(1).R*tx(1).WPE*power_tx(:,j)'*h(:,j);
                    
                    power_tx_interfer = power_tx;
                    power_tx_interfer(:,j) = [];
                    h_interfer = h(:,j);
                    sig_interfer = rx(1).R*tx(1).WPE*sum(power_tx_interfer'*h_interfer);
                    PTx(i) = sum(sum(power_tx)); 
                    SINR(i,j) = sig_desired^2/(comm.getRxNoisePower() + sig_interfer^2);
                end
            end
            SINR = SINR';
            
            PTx = [0 PTx]; %add zero transmission power
            SINR = [zeros(no_rx,1) SINR];
            s = [zeros(no_tx*no_rx,1) s];
            TP_system = 0;
            logTP_system = 0;
            for i=1:size(SINR,1)
                TP_system = TP_system + B*log2(SINR(i,:)+1);
                logTP_system = logTP_system + log10(B*log2(SINR(i,:)+1));
            end
        end
    end
    
end