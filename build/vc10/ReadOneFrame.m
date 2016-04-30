ImgFileName = char(input('Image File:'));
ImgWidth = uint16(input('Image Width:'));
ImgHeight = uint16(input('Image Height:'));

fid = fopen(ImgFileName);
C1 = uint8(fread(fid,[ImgWidth ImgHeight],'uint8'))';
C2 = uint8(fread(fid,[ImgWidth ImgHeight],'uint8'))';
C3 = uint8(fread(fid,[ImgWidth ImgHeight],'uint8'))';
fclose(fid);

disp('Done.');
str = sprintf('Image File:%s',ImgFileName);
disp(str);
str = sprintf('Image Size:%dx%d',ImgWidth,ImgHeight);
disp(str);