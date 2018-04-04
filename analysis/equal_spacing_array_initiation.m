clear


npolymer = 300;
nmonomer = 2;
point_spacing = 0.5;
actin_length = 0.5;
xdim = 10;
ydim = 10;
top_corner = ([0 - xdim/2, ydim - ydim/2]);
monomer_spacing = 1;
pol_per_row = floor(xdim/(monomer_spacing*(nmonomer))); %- 1;
row_spacing = (ydim/(npolymer/pol_per_row));

%initiate the array
equal_lattice = zeros([npolymer*nmonomer 4]);

%set the easy variables first
equal_lattice(:,3) = actin_length;

%set polymer index
for x = 1:npolymer
    for y = 1:nmonomer
        equal_lattice((x-1)*nmonomer + y, 4) = (x-1);
    end
end

for a = 1:npolymer
    for b = 1:nmonomer
       
        if mod(a,pol_per_row) == 1
        equal_lattice((a-1)*nmonomer + b, 1) = top_corner(1) +...
            monomer_spacing*(b-1);
        elseif mod(a,pol_per_row) ~= 0
        equal_lattice((a-1)*nmonomer + b, 1) = top_corner(1) +...
            monomer_spacing*(b-1) +...
            (xdim/pol_per_row)*(mod(a,pol_per_row) - 1);
        else
        equal_lattice((a-1)*nmonomer + b, 1) = top_corner(1) +...
            monomer_spacing*(b-1) +...
            (xdim/pol_per_row)*(pol_per_row - 1);
        end
        
        equal_lattice((a-1)*nmonomer + b, 2) = top_corner(2) -...
            row_spacing*(ceil(a/pol_per_row) - 1);
        
    end
    
end



