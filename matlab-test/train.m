clear all 
close all
clc

% Data Import (it takes a while)

data = readtable('comp3208-2017-train.csv');
data = data{:,:};

%% 

N = 8649;

SIM = diag(ones(1,N));

J = zeros(1.5e4,3*N);

lastIndex_j = 48;
i = 1;
c = 2;
for j = 2:3:3*N
        J(:,i:i+2) = data(lastIndex_j:lastIndex_j+1.5e4-1,:);
        lastIndex_j = lastIndex_j + sum(J(:,i) == c);
        c = c + 1;
        i = i + 3;
end

lastIndex_i = 1;
for i = 1:N
    temp1 = data(lastIndex_i:lastIndex_i+1.5e4,:);
    user1 = temp1(temp1(:,1) == i,2:3);
    lastIndex_j = lastIndex_i + size(user1,1);
    c = 2;
    col = 1;
    for j = 2:3:3*N-3
        %temp2 = data(lastIndex_j:15e4,:);
        user2 = J(J(:,col) == c,j:j+1);
        SIM(i,c) = evaluatePearson(user1,user2);
        SIM(c,i) = SIM(i,c);
        lastIndex_j = lastIndex_j + size(user2,1);
        c = c + 1;
        col = col + 3;
    end
    
    lastIndex_i = lastIndex_i + size(user1,1);
    
end

save('SIM','SIM');