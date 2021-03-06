#! /usr/bin/env python
"""
Module that permits generating reports by reading properties files
"""

from __future__ import with_statement, division

import os
import sys
import shutil
import re
from glob import glob
from collections import defaultdict
from itertools import combinations
import logging
import datetime
import collections
import cPickle


                    
import tools
from markup import Document
from external.configobj import ConfigObj
from external.datasets import DataSet
from external import txt2tags
from external import argparse


def avg(values):
    """Computes the arithmetic mean of a list of numbers.

    >>> print avg([20, 30, 70])
    40.0
    """
    return round(sum(values, 0.0) / len(values), 4)
    
    
def gm(values):
    """Computes the geometric mean of a list of numbers.

    >>> print gm([2, 8])
    4.0
    """
    assert len(values) >= 1
    return round(tools.prod(values) ** (1/len(values)), 4)


class ReportArgParser(tools.ArgParser):
    def __init__(self, *args, **kwargs):
        tools.ArgParser.__init__(self, *args, add_help=True, **kwargs)
      
        self.add_argument('source', help='path to results directory', 
                            type=self.directory)
        
        self.add_argument('-d', '--dest', dest='report_dir', default='reports',
                            help='path to report directory')
                        
        self.add_argument('--format', dest='output_format', default='html',
                            help='format of the output file',
                            choices=sorted(txt2tags.TARGETS))
                            
        self.add_argument('--group-func', default='sum',
                        help='the function used to cumulate the values of a group')
                        
        self.add_argument('--hide-sum', dest='hide_sum_row',
                        default=False, action='store_true',
                        help='do not add a row that sums up each column')
                        
        self.add_argument('--dry', default=False, action='store_true',
                        help='do not write anything to the filesystem')
                        
        self.add_argument('--reload', default=False, action='store_true',
                        help='rescan the directory and reload the properties files')
                        
        self.add_argument('--show_attributes', default=False, action='store_true',
                        help='show a list of available attributes and exit')
                        
        self.add_argument('-a', '--attributes', dest='foci', nargs='*', 
                            metavar='ATTR', default='all',
                            help='the analyzed attributes (e.g. "expanded")')
                        
                        
    def parse_args(self, *args, **kwargs):
        args = tools.ArgParser.parse_args(self, *args, **kwargs)
        
        args.eval_dir = args.source
        
        args.eval_dir = os.path.normpath(os.path.abspath(args.eval_dir))
        logging.info('Eval dir: "%s"' % args.eval_dir)
        
        if not args.eval_dir.endswith('eval'):
            answer = raw_input('The source directory does not end with eval. '
                        'Are you sure you this is an evaluation directory? (Y/N): ')
            if not answer.upper() == 'Y':
                sys.exit()
        
        if not os.path.exists(args.report_dir):
            os.makedirs(args.report_dir)
            
        # Turn e.g. the string 'max' into the function max()
        args.group_func = eval(args.group_func)
        
        #if not args.report_dir:
        #    parent_dir = os.path.dirname(args.eval_dir)
        #    dir_name = os.path.basename(args.eval_dir)
        #    args.report_dir = os.path.join(parent_dir, dir_name + '-report')
        #    logging.info('Report dir: "%s"' % args.report_dir)
        
        return args
        
    

class Report(object):
    """
    Base class for all reports
    """
    def __init__(self, parser=ReportArgParser()):
        #Define which attribute the report should be about
        #self.focus = None
        
        # Give all the options to the report instance
        parser.parse_args(namespace=self)
        
        self.data = None
        self.orig_data = self._get_data()
        
        if not self.foci or self.foci == 'all':
            self.foci = self.orig_data.get_attributes()
        
        logging.info('Attributes: %s' % self.foci)
        
        if self.show_attributes:
            print
            print 'Available attributes:'
            print self.orig_data.get_attributes()
            sys.exit()
        
        self.filter_funcs = []
        self.filter_pairs = {}
        
        self.grouping = []
        self.order = []
        
        self.infos = []
        
        
    def add_filter(self, *filter_funcs, **filter_pairs):
        self.filter_funcs.extend(filter_funcs)
        self.filter_pairs.update(filter_pairs)
        
        
    def set_grouping(self, *grouping):
        """
        Set by which attributes the runs should be separated into groups
        
        grouping = None/[]: Use only one big group (default)
        grouping = 'domain': group by domain
        grouping = ['domain', 'problem']: Use one group for each problem
        """
        self.grouping = grouping
        
        
    def set_order(self, *order):
        self.order = order
        
    def add_info(self, info):
        """
        Add strings of additional info to the report
        """
        self.infos.append(info)
        
       
    @property
    def group_dict(self):
        data = DataSet(self.data)
        
        if not self.order:
            self.order = ['id']
        data.sort(*self.order)
        #print 'SORTED'
        #data.dump()
        
        if self.filter_funcs or self.filter_pairs:
            data = data.filtered(*self.filter_funcs, **self.filter_pairs)
            #print 'FILTERED'
            #data.dump()
        
        group_dict = data.group_dict(*self.grouping)
        #print 'GROUPED'
        #print group_dict
                
        return group_dict
        
        
    def _get_data(self):
        """
        The data is reloaded for every attribute
        """
        dump_path = os.path.join(self.eval_dir, 'data_dump')
        
        dump_exists = os.path.exists(dump_path)
        # Reload when the user requested it or when no dump exists
        if self.reload or not dump_exists:
            data = DataSet()
            logging.info('Started collecting data')
            for base, dir, files in os.walk(self.eval_dir):
                for file in files:
                    if file == 'properties':
                        file = os.path.join(base, file)
                        props = tools.Properties(file)
                        data.append(**props)
            # Pickle data for faster future use
            cPickle.dump(data, open(dump_path, 'w'))
            logging.info('Wrote data dump')
            logging.info('Finished collecting data')
        else:
            data = cPickle.load(open(dump_path))
        return data
        
        
    def name(self):
        name = ''
        eval_dir = os.path.basename(self.eval_dir)
        name += eval_dir.replace('-', '')
        if len(self.foci) == 1:
            name += '-' + self.foci[0]
        return name
        
    
    def _get_table(self):
        raise Error('Not implemented')
        
        
    def write(self):
        doc = Document(title=self.name())
        string = str(self)
        
        if not string:
            logging.info('No tables generated. ' \
                        'This happens when no significant changes occured. ' \
                        'Therefore no output file has been created')
            return
            
        doc.add_text(string)
        
        self.output = doc.render(self.output_format, {'toc': 1})
            
        if not self.dry:
            ext = 'html' if self.output_format == 'xhtml' else self.output_format
            #date = datetime.datetime.now().strftime("%Y-%m-%d_%H:%M:%S")
            output_file = os.path.join(self.report_dir, self.name() + '.' + ext)
            with open(output_file, 'w') as file:
                logging.info('Writing output to "%s"' % output_file)
                file.write(self.output)
        
        logging.info('Finished writing report')
        
        
    def __str__(self):
        res = ''
        for info in self.infos:
            res += '- %s\n' % info
        if self.infos:
            res += '\n\n====================\n'
        
        tables = []
        for focus in self.foci:
            self.data = self.orig_data.copy()
            self.focus = focus
            try:
                table = self._get_table()
                if table:
                    # We return None for a table if we don't want to add it
                    print table
                    tables.append(table)
            except TypeError, err:
                logging.info('Omitting attribute "%s" (%s)' % (focus, err))
                    
        if not tables:
            return ''
            
        for table in tables:
            res += '+ %s +\n%s\n' % (self.focus, table)
            
        return res

                            
        

class Table(collections.defaultdict):
    def __init__(self, title='', highlight=True, min_wins=True, numeric_rows=False):
        collections.defaultdict.__init__(self, dict)
        
        self.title = title
        self.highlight = highlight
        self.min_wins = min_wins
        self.numeric_rows = numeric_rows
        
        
    def add_cell(self, row, col, value):
        self[row][col] = value
        
        
    @property    
    def rows(self):
        special_rows = ['SUM', 'AVG', 'GM']
        rows = self.keys()
        # Let the sum, etc. rows be the last ones
        if self.numeric_rows:
            key = lambda row: int(row)
        else:
            key = lambda row: 'zzz'+row.lower() if row.upper() in special_rows else row.lower()
        rows = sorted(rows, key=key)
        return rows
        
        
    @property
    def cols(self):
        cols = []
        for dict in self.values():
            for key in dict.keys():
                if key not in cols:
                    cols.append(key)
        # Put special columns at the end
        key = lambda col: 'zzz'+col.lower() if col.lower() in ['quotient'] else col.lower()
        return sorted(cols, key=key)
        
        
    def get_cells_in_row(self, row):
        return [self[row][col] for col in self.cols]
        
        
    def get_relative(self):
        """
        Take the first value of each row and divide every value in the row by it
        Returns a new table
        
        Unused for now
        """
        rel_table = Table(self.title)
        col1 = self.cols[0]
        for row in self.rows:
            val1 = self[row][col1]
            for col, cell in self[row].iteritems():
                rel_value = 0 if val1 == 0 else round(cell / val1, 4)
                rel_table.add_cell(row, col, rel_value)
        return rel_table
        
        
    def get_comparison(self, comparator=cmp):
        """
        || expanded                      | fF               | yY               |
        | **prob01.pddl**                | 21               | 16               |
        | **prob02.pddl**                | 38               | 24               |
        | **prob03.pddl**                | 59               | 32               |
        ==>
        returns ((fF, yY), (0, 0, 3)) [wins, draws, losses]
        """
        assert len(self.cols) == 2, 'For comparative reports please specify 2 configs'
        
        sums = [0, 0, 0]
        
        for row in self.rows:
            for col1, col2 in combinations(self.cols, 2):
                val1 = self[row][col1]
                val2 = self[row][col2]
                cmp_value = comparator(val1, val2)
                sums[cmp_value + 1] += 1
                    
        return (self.cols, sums)
        
    
    def get_row(self, row, values=None):
        '''
        values has to be sorted by the corresponding column names
        '''
        if values is None:
            values = []
            for col in self.cols:
                values.append(self.get(row).get(col))
            
        only_one_value = len(set(values)) == 1
            
        min_value = min(values)
        max_value = max(values)
        
        min_wins = self.min_wins
        max_wins = not min_wins
            
        text = ''
        if self.numeric_rows:
            text += '| %-30s ' % (row)
        else:
            text += '| %-30s ' % ('**'+row+'**')
        for value in values:
            is_min = (value == min_value)
            is_max = (value == max_value)
            if self.highlight and only_one_value:
                value_text = '{{%s|color:Gray}}' % value
            elif self.highlight and (min_wins and is_min or max_wins and is_max):
                value_text = '**%s**' % value
            else:
                value_text = str(value)
            text += '| %-16s ' % value_text
        text += '|\n'
        return text
    
        
    def __str__(self):
        """
        {'zenotravel': {'yY': 17, 'fF': 21}, 'gripper': {'yY': 72, 'fF': 118}}
        ->
        || expanded        | fF               | yY               |
        | **gripper     ** | 118              | 72               |
        | **zenotravel  ** | 21               | 17               |
        """
        text = '|| %-29s | ' % self.title
        
        rows = self.rows
        cols = self.cols
        
        text += ' | '.join(map(lambda col: '%-16s' % col, cols)) + ' |\n'
        for row in rows:
            text += self.get_row(row)
        return text
        
    

if __name__ == "__main__":
    logging.error('Please import this module from another script')
