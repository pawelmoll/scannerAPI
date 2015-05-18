from sys import version_info
if version_info >= (2,6,0):
    from os.path import dirname
    import imp

    fp = None
    fp, pathname, description = imp.find_module('pyscanner', [dirname(__file__)])

    try:
        pyscanner = imp.load_module('pyscanner', fp, pathname, description)
    finally:
        if fp is not None:
            fp.close()
    del fp
    del pathname
    del description
else:
        import pyscanner
del version_info



def list():
    return pyscanner.list()



class scanner:
    def __init__(self, name):
        self.__scanner = pyscanner.get(name)
        pyscanner.on(self.__scanner)
        self.__caps = pyscanner.get_caps(self.__scanner)

    def __del__(self):
        pyscanner.off(self.__scanner)
        del self.__scanner

    def scan(self, timeout):
        return pyscanner.scan(self.__scanner, timeout)

    def image(self):
        if not 'image_format' in self.__caps:
            return None
        return { 'format': self.__caps['image_format'],
                'width': self.__caps['image_width'],
                'height': self.__caps['image_height'],
                'data': pyscanner.get_image(self.__scanner) }

    def iso_template(self):
        if not 'iso_template' in self.__caps or not self.__caps['iso_template']:
            return None
        return pyscanner.get_iso_template(self.__scanner)
