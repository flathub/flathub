% ActiveFEMM (C)2006 David Meeker, dmeeker@ieee.org

function z=callfemm(x)
    global ifile ofile HandleToFEMM

if (exist('actxserver'))
	
	z=invoke(HandleToFEMM,'mlab2femm',x);
	if (length(z)~=0)
    	if (z(1)=='e')
        	error(sprintf('FEMM returns:\n%s',z));
    	else
        	z=strrep(z,'I','i');
       	 	z=eval(z);
    	end
	end

else

    % Form that I'd previously used in Octave:
	try
    	[fid,msg]=fopen(ifile,'wt');
	catch
		[fid,msg]=fopen(ifile,'w');
	end
    fprintf(fid,'flput(%s)',x);
    fclose(fid);
    do
        do
			try
	            [fid,msg]=fopen(ofile,'rt');
			catch
				[fid,msg]=fopen(ofile,'r');
			end
            if (fid==-1) pause(0.001); end
        until (fid~=-1)
        u=fgets(fid);
        fclose(fid);
    until (u~=-1)
    pause(0.001);
    unlink(ofile);
    z=eval(u);

end

