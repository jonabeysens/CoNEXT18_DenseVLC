classdef room
    properties
        id
        dim                             % Room dimensions [x y z]
        resolution                      %resolution of room dimensions
        no_tx_horz
        no_tx_vert
    end
    methods
        %% Constructor
        function obj = room(id,dim,no_tx_horz,no_tx_vert)
            obj.id = id;
            obj.dim = dim; %[x y z] coordinate
            obj.resolution = 0.01;
            obj.no_tx_horz = no_tx_horz;
            obj.no_tx_vert = no_tx_vert;
        end
        
        %% Get functions
        function dim = getDimensions(obj)
            dim = obj.dim;
        end
        
        function center = getCenter(obj)
            center = obj.dim./2;
        end
        
        function resolution = getResolution(obj)
            resolution = obj.resolution;
        end
        
        %% Set functions
        function obj = setDimensions(obj,dim)
            obj.dim = dim;
        end
        
        
    end
end

