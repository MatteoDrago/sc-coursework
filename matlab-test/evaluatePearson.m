function ps = evaluatePearson(user1,user2)
    
    % Average ratings of each user
    avg1 = mean(user1(:,2));
    avg2 = mean(user2(:,2));
    
    % First condition for no matching items
    if(max(user1(:,1)) < min(user2(:,1)) || max(user2(:,1)) < min(user1(:,1)))
        ps = NaN;
        return;
    end
    
    c = 1;
    r1 = NaN;
    r2 = NaN;
    
    if size(user2(:,1),1) < size(user1(:,1),1)
 
        for i = 1:size(user2(:,1),1)
            idxRat = user1(:,1) == user2(i,1);
            if any(idxRat)
                r1(c) = user1(idxRat,2);
                r2(c) = user2(i,2);
                c = c + 1;
            end
        end
        
    else
        
        for i = 1:size(user1(:,1),1)
            idxRat = user2(:,1) == user1(i,1);
            if any(idxRat)
                r1(c) = user1(i,2);
                r2(c) = user2(idxRat,2);
                c = c + 1;
            end
        end
        
    end
    
    
    diff1 = r1 - avg1;
    diff2 = r2 - avg2;
    
    num = diff1 * diff2';
    den = sqrt(sum(diff1.^2))*sqrt(sum(diff2.^2))';
    ps = num/den;