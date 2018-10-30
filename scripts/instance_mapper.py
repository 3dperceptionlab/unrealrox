""" This script takes an instance-to-class mapping from a sequence and asks the user to map each one
of the instances to a class of a given set of possible classes to generate a new mapping."""

__author__      = "Alberto Garcia-Garcia"
__copyright__   = "Copyright 2018, 3D Perception Lab"
__credits__     = ["Alberto Garcia-Garcia",
                    "Pablo Martinez-Gonzalez",
                    "Sergiu Oprea",
                    "Alvaro Jover-Alvarez",
                    "John Castro-Vargas",
                    "Sergio Orts-Escolano",
                    "Jose Garcia-Rodriguez"]

__license__     = "MIT"
__version__     = "1.0"
__maintainer__  = "Alberto Garcia-Garcia"
__email__       = "agarcia@dtic.ua.es"
__status__      = "Development"

import argparse
import json
import logging
import sys

log = logging.getLogger(__name__)

def choose_class(classes):

    class_names_ = list(classes.keys())
    num_classes_ = len(class_names_)

    for i in range(num_classes_):
        log.info("({0}) {1}".format(i, class_names_[i]))

    election_ = "-1"

    while (int(election_) < 0 or int(election_) > num_classes_):
        election_ = input("Choose a class (k: to keep the previous class; s: to stop mapping): ")

        if (election_ == "k"):
            return election_
        elif (election_ == "s"):
            return election_

    return class_names_[int(election_)]

def mapper(args):

    # Load classes
    classes_ = {}
    with open(args.classes) as f:
        classes_ = json.load(f)

    # Load current instance-class mapping
    instance_classs_ = {}
    with open(args.seq_mapping) as f:
        instance_class_ = json.load(f)

    # Iterate over objects
    for i in range(len(instance_class_["SceneObjects"])):

        log.info("Object {0}".format(instance_class_["SceneObjects"][i]["instance_name"]))
        log.info("Belongs to class {0}...".format(instance_class_["SceneObjects"][i]["class"]))

        obj_class_ = choose_class(classes_)

        if (obj_class_ == "k"):
            log.info("Kept the previous class...")
        elif (obj_class_ == "s"):
            log.info("Stop mapping...")
            break
        else:
            instance_class_["SceneObjects"][i]["class"] = obj_class_

    # Save new instance-class mapping
    with open(args.out_mapping, 'w') as f:
        f.write(json.dumps(instance_class_, indent=2))

if __name__ == "__main__":

    logging.basicConfig(stream=sys.stdout, level=logging.INFO)

    parser_ = argparse.ArgumentParser(description='Parameters')
    parser_.add_argument('--classes', nargs='?', type=str, default='classes.json', help='The classes description file.')
    parser_.add_argument('--seq_mapping', nargs='?', type=str, default='sceneObject.json', help='The JSON file containing instance to class mapping for the sequence.')
    parser_.add_argument('--out_mapping', nargs='?', type=str, default='instance_class_mapped.json', help='The JSON file to output the instance to class mapping for the sequence.')

    args_ = parser_.parse_args()
    mapper(args_)