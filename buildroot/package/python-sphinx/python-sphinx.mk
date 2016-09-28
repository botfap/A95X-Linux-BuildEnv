################################################################################
#
# sphinx
#
################################################################################
PYTHON_SPHINX_VERSION       = 1.2.2
PYTHON_SPHINX_SOURCE        = Sphinx-$(PYTHON_SPHINX_VERSION).tar.gz
PYTHON_SPHINX_SITE          = https://pypi.python.org/packages/source/S/Sphinx/
PYTHON_SPHINX_LICENSE       = Python software foundation license v2, others
PYTHON_SPHINX_LICENSE_FILES = LICENSE
PYTHON_SPHINX_SETUP_TYPE = setuptools

$(eval $(python-package))
$(eval $(host-python-package))
