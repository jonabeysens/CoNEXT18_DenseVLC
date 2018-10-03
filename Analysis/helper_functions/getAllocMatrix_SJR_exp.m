function alloc_matrix = getAllocMatrix_SJR_exp(swings,exponent)
% Get the allocation matrix (=matrix containing which TX should assigned first) based on the swing levels
    no_tx = size(swings,1);
    no_rx = size(swings,2);

    h = swings;
    for i=1:no_tx
        for j=1:no_rx
            h_jam = h(i,:);
            h_jam(j) = [];
            SJR(i,j) = h(i,j)^exponent/(h(i,j) + sum(h_jam));
            if(isnan(SJR(i,j)))
                SJR(i,j) = -inf;
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
