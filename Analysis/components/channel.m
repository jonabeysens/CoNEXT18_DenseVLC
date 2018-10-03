classdef channel
    properties
    end
    
    methods

        %% Constructor
        function obj = channel()
        end
        
        %% Distance related aspects       
        function [dist,cosPhi,cosPsi] = getDistance_1TX_PD(obj,tx_single,rx)
            dist_vec = [tx_single.pos(1)-rx.pos(1) tx_single.pos(2)-rx.pos(2) tx_single.pos(3)-rx.pos(3)];
            dist = norm(dist_vec);
            cosPhi = dot(tx_single.orient,-dist_vec)/(norm(tx_single.orient)*dist); %-dist_vec because otherwise we don't get a positive value for cosPhi
            cosPsi = dot(rx.orient,dist_vec)/(norm(rx.orient)*dist);
        end

        function [indx,indy,dist,cosPhi,cosPsi] = getDistance2D(obj,tx_single,rx_single,room,spatial_range)
            % GETDISTANCE2D get distance from a single transmitter to a single receiver
            %   room = room object needed to fetch room resolution
            %   spatial_range =  the range over which to compute the distance
            %   
            indx = spatial_range(1,1):room.getResolution():spatial_range(1,2);
            indy = spatial_range(2,1):room.getResolution():spatial_range(2,2);
            indz = rx_single.pos(3);
            [xx,yy,zz] = meshgrid(indx,indy,indz);
            
            dist = sqrt((tx_single.pos(1)-xx).^2 + (tx_single.pos(2)-yy).^2 + (tx_single.pos(3)-zz).^2);
            cosPhi = (tx_single.orient(1) .* -(tx_single.pos(1)-xx) + tx_single.orient(2) .* -(tx_single.pos(2)-yy) + tx_single.orient(3) .* -(tx_single.pos(3)-zz))./dist; % - sign otherwise not positive value for cosPhi
            cosPsi = (rx_single.orient(1) .* (tx_single.pos(1)-xx) + rx_single.orient(2) .* (tx_single.pos(2)-yy) + rx_single.orient(3) .* (tx_single.pos(3)-zz))./dist; 
        end
        
        %% Channel impulse response       
        function pathLoss = getPathLoss(obj,tx,rx_single) % channel impulse response from all Tx to single PD  (= DC channel gain, path loss) 
            for i=1:length(tx)
                [dist,cosPhi,cosPsi] = getDistance_1TX_PD(obj,tx(i),rx_single);
                radPatternPhi = tx(i).radPattern_cont(acosd(cosPhi));
                if radPatternPhi < 0
                    radPatternPhi = 0;
                end
                if abs(acosd(cosPsi)) <= rx_single.fov
                    lumFlux_rx = tx(i).getLumIntensCenter() * (radPatternPhi * rx_single.activeArea *  cosPsi )/ dist^2;
                else
                    lumFlux_rx = 0;
                end
                lumFlux_tx = tx(i).getLumFlux_lumIntensCenter();
                pathLoss(i) = lumFlux_rx/lumFlux_tx;
            end
        end  
        
        
        %% Illuminance related aspects  
        function [EvRoom_total,EvRoom_sep] = getLuxRoom(obj,tx,rx,room)
            spatial_range = [0 room.dim(1) ; 0 room.dim(2)];
            [~,~,EvRoom_total,EvRoom_sep] = getLux2D(obj,tx,rx,room,spatial_range);
        end
        function [indx,indy,EvRegion_total,EvRegion_sep] = getLux2D(obj,tx,rx,room,spatial_range)
            for i=1:length(tx)                
                [indx,indy,dist,cosPhi,cosPsi] = getDistance2D(obj,tx(i),rx,room,spatial_range);
                radPatternPhi = reshape(tx(i).radPattern_cont(acosd(cosPhi)),size(cosPhi,1),size(cosPhi,2)); % radiation pattern
                
                lumIntens(:,:,i) = tx(i).lumIntensCenter.*radPatternPhi; % luminuous intensity
                Ev(:,:,i) = lumIntens(:,:,i).*cosPsi./(dist.^2);   
            end
            EvRegion_total = sum(Ev,3);
            EvRegion_sep = Ev;
        end
        
        function [indx,Ev_intersec] = getLux1D(obj,intersec_y,tx,rx,room)
            indx = 0:room.getResolution():room.dim(1);
            indy = 0:room.getResolution():room.dim(2);
            
            [~,indy_intersec] = min(abs(indy-intersec_y));
            
            [EvRoom_total,~] = getLuxRoom(obj,tx,rx,room);
            
            Ev_intersec = EvRoom_total(indy_intersec,:);
         end 
        
        %% visualization functions
         function EvRoom_total = visLuxRoom(obj,tx,rx,room,newfig)
            if newfig; figure; end;
            indx = 0:room.getResolution():room.dim(1);
            indy = 0:room.getResolution():room.dim(2);
            
            [EvRoom_total,~] = getLuxRoom(obj,tx,rx,room);
            
            imagesc(indx,indy,EvRoom_total);
            set(gca, 'Ydir', 'normal');
            colormap jet;
            c = colorbar;
            c.Label.String = 'Illuminance [lux]';
            xlabel('position in x dimension [m]');
            ylabel('position in y dimension [m]');
         end
    end
end

