classdef comm
    properties
        modScheme
        no_rx
        M
        rxNoisePower
    end
    
    methods
        function obj = comm(modScheme,no_rx)
            if(strcmp(modScheme,'OOK'))
                obj.M = 2;
            end
            obj.modScheme = modScheme;
            obj.no_rx = no_rx;          
        end

%% getters
        function rxNoisePower = getRxNoisePower(obj)
            rxNoisePower = obj.rxNoisePower;
        end
        
%% Setters
        function obj = setRxNoisePower(obj,N0,B)
            obj.rxNoisePower = N0*B;
        end
        
    end
end