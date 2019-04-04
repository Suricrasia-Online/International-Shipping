
all : main

.PHONY: clean

packer : vondehi/vondehi.asm 
	cd vondehi; nasm -fbin -o vondehi vondehi.asm

shader.frag.min : shader.frag Makefile
	cp shader.frag shader.frag.min
	sed -i 's/m_origin/o/g' shader.frag.min
	sed -i 's/m_direction/d/g' shader.frag.min
	sed -i 's/m_point/k/g' shader.frag.min
	sed -i 's/m_intersected/i/g' shader.frag.min
	sed -i 's/m_color/c/g' shader.frag.min
	sed -i 's/m_mat/m/g' shader.frag.min
	sed -i 's/m_cumdist/y/g' shader.frag.min
	sed -i 's/m_attenuation/l/g' shader.frag.min

	sed -i 's/m_diffuse/o/g' shader.frag.min
	sed -i 's/m_specular/d/g' shader.frag.min
	sed -i 's/m_spec_exp/k/g' shader.frag.min
	sed -i 's/m_reflectance/i/g' shader.frag.min
	sed -i 's/m_transparency/c/g' shader.frag.min

	sed -i 's/MAXDEPTH/5/g' shader.frag.min

	sed -i 's/\bRay\b/Co/g' shader.frag.min
	sed -i 's/\bMat\b/Cr/g' shader.frag.min

shader.h : shader.frag.min Makefile
	mono ./shader_minifier.exe shader.frag.min -o shader.h

# not using `pkg-config --libs` here because it will include too many libs
main.elf : main.c shader.h Makefile
	gcc -o $@ $< `pkg-config --cflags gtk+-3.0` -lm -lGL -lgtk-3 -lgdk-3 -lgobject-2.0 -lfftw3f -no-pie -fno-plt -Os -std=gnu11 -nostartfiles #-nostdlib

main : main_opt.elf.packed
	mv $< $@

#all the rest of these rules just takes a compiled elf file and generates a packed version of it with vondehi
%_opt.elf : %.elf Makefile
	cp $< $@
	strip $@
	strip -R .note -R .comment -R .eh_frame -R .eh_frame_hdr -R .note.gnu.build-id -R .got -R .got.plt -R .gnu.version -R .rela.dyn -R .shstrtab $@
	#remove section header
	./Section-Header-Stripper/section-stripper.py $@

	#clear out useless bits
	sed -i 's/_edata/\x00\x00\x00\x00\x00\x00/g' $@;
	sed -i 's/__bss_start/\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/g' $@;
	sed -i 's/_end/\x00\x00\x00\x00/g' $@;

	wc -c $@
	chmod +x $@

%.xz : % Makefile
	-rm $@
	lzma --format=lzma -9 --extreme --lzma1=preset=9,lc=1,lp=0,pb=1,nice=270,depth=64,dict=16384 --keep --stdout $< > $@
	wc -c $@

%.packed : %.xz packer Makefile
	cat ./vondehi/vondehi $< > $@
	chmod +x $@
	wc -c $@

clean :
	-rm *.elf shader.h main
