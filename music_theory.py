def majors(): 
    for x in range(12):
        names, reps, min_acc =[('A','A'), ('Bb','A#'), ('B','Cb'), ('C','B#'), ('Db','C#'), ('D','D'),
            ('Eb','D#'), ('E','Fb'), ('F','E#'), ('Gb','F#'), ('G','G'), ('Ab','G#')], set(), 6
        for bitmask in range(2**7):
            s = ' '.join([names[[(i+x)%12 for i in [0,2,4,5,7,9,11]][i]][(bitmask & 2**i)>>i] for i in range(7)])
            if not (False in [note in s for note in ['A','B','C','D','E','F','G']]):
                min_acc = min(min_acc, s.count('b') + s.count('#'))
                reps.add(s)
        for r in filter(lambda s: s.count('b') + s.count('#') == min_acc, reps):
            print("{} Major:\t{} flats, {} sharps\n{}\n".format(r[:r.find(' ')], r.count('b'), r.count('#'), r))
