classdef receiver
    properties
        id
        name                                    % Name of LED
        lumEfficacy                             % Luminous efficacy
        activeArea                              % Active area receiver 
        fov                                     % Field of vieuw
        R                                       % Responsivity of PD (optical -> electrical power)
        pos
        angle                                   % angle[1] = Elevation angle of receiver / angle[2] = Azimuth angle of receiver
        orient                                  % Orientation of receiver 
    end
    methods
        %% Constructor
        function obj = receiver(id,angle,height,room)
            % general settings
            obj.id = id;
            obj.name = '';
            
            % PD properties
            obj.fov = 90;
            obj.activeArea = 1.1e-6;

            % communication related aspects
            obj.R = 0.4; %in ampere/Watt
            
            % geometric settings
            center = room.getCenter();
            obj.pos = [center(1) center(2) height];
            obj.orient = getOrientation(obj,angle(1),angle(2));
        end
        
        function id = getID(obj)
            id = obj.id;
        end
        
        %% Functions
        % Determine orientation unit vector of receiver
        function orient = getOrientation(obj,angle_elev,angle_azim)
            orient = [cosd(angle_azim)*cosd(angle_elev) sind(angle_azim)*cosd(angle_elev) sind(angle_elev)];
        end
        
        function position = getPosition(obj)
            position = obj.pos;
        end
        
        function obj = setPosition(obj,room,position)
            if(any([position obj.pos(3)] > room.getDimensions()))
                error('position out of range');
            end
            obj.pos = [position(1) position(2) obj.pos(3)];
        end
        
        
        function opticalPower = getOpticalPower(obj,tx,channel)
            pathLoss = channel.getPathLoss(tx,obj);
            for i=1:length(tx)
                opticalPower_tx = tx(i).getOpticalPower();
                opticalPower(i) = pathLoss(i)*opticalPower_tx;
            end
        end
    end
end